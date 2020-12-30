#define YAML_PARSER_PRIVATE
#include "saucer.h"

static int32_t op_key_ws_c = -1;
static char *multi_line_str = NULL;
static int32_t ml_str_index = 0;

/* Generic yaml parser helpers */
/* Check if line contains string */
static uint8_t yamlSearchString(char *line, int32_t line_index, StrBounds **p_str_bds, int32_t *p_str_bds_c) {
    int32_t index;
    CharInfo *qms = (CharInfo*) calloc (
        1, 
        sizeof(CharInfo)
    );
    
    int32_t qm_c = 0;
    uint8_t is_str_found = false;
    
    // Find the minimum quotation mark index
    for(index = 0; index < strlen(line); index++) {
        if(line[index] == 0x22 || line[index] == 0x27) {
            qm_c++;
            CharInfo *p_tmp = (CharInfo*) realloc (
                qms, 
                qm_c * sizeof(CharInfo)
            );

            if(!p_tmp) {
                MEMERR("failed to reallocate memory for quomark charinfos");
                exit(ERRC_MEM);
            }

            qms = p_tmp;  
            qms[qm_c - 1].ch = line[index];
            qms[qm_c - 1].index = index;
            is_str_found = true;
        }
    }

    int32_t s_quo_c = 0;
    int32_t d_quo_c = 0;
    
    // Count the total amount of quotation marks 
    for(index = 0; index < qm_c; index++) {
        if(qms[index].ch == 0x27) s_quo_c++;
        else if(qms[index].ch == 0x22) d_quo_c++;
    }

    // Check for parsing errors
    if(s_quo_c % 2) {
        PERR("unclosed single quotation mark string", line_index + 1);
        exit(ERRC_PARSE);
    }

    if(d_quo_c % 2) {
        PERR("unclosed double quotation mark string", line_index + 1);
        exit(ERRC_PARSE);
    }

    for(index = 0; index < qm_c - 1; index += 2) {
        // Check for parsing error
        if(qms[index].ch != qms[index + 1].ch) {
            PERR("quotation marks (0x27 and 0x22) are not allowed to be used inside strings", line_index + 1);
            exit(ERRC_PARSE);
        }

        (*p_str_bds_c)++;
        
        StrBounds *p_tmp = (StrBounds*) realloc (
            (*p_str_bds), 
            (*p_str_bds_c) * sizeof(StrBounds)
        );

        if(!p_tmp) {
            MEMERR("failed to reallocate memory for string bounds");
            exit(ERRC_MEM);
        }
        
        (*p_str_bds) = p_tmp;
        (*p_str_bds)[(*p_str_bds_c) - 1].str_beg = qms[index].index;
        (*p_str_bds)[(*p_str_bds_c) - 1].str_end = qms[index + 1].index;
    }

    free(qms);
    return is_str_found;
}


/* Find the key name colon positions and name beginning breakpoint position on line */
static uint8_t yamlFindKeyNameBounds(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyPosInfo **p_kpi, int32_t *p_kpi_c) {
    int32_t l_index, r_index;
    uint8_t found_min = false;
    uint8_t is_array_beg = false;
    uint8_t is_array_cl = false;

    // Search for colons
    uint8_t is_colon = false;
    for(l_index = min_index; l_index < max_index; l_index++) {
        if(line[l_index] == 0x3A) {
            is_colon = true;
            (*p_kpi_c)++;
            KeyPosInfo *p_tmp = (KeyPosInfo*) realloc (
                (*p_kpi), 
                (*p_kpi_c) * sizeof(KeyPosInfo)
            );

            if(!p_tmp) {
                MEMERR("failed to reallocate memory for KeyPosInfo");
                exit(ERRC_MEM);
            }

            (*p_kpi) = p_tmp;
            (*p_kpi)[(*p_kpi_c) - 1].max_index = l_index;
        
            // Check if the colon represents a key
            if
            (
                l_index + 1 < max_index && 
                line[l_index + 1] != 0x20
            ) return false; 
        }        

        if(line[l_index] == 0x7B) is_array_beg = true;
        if(line[l_index] == 0x7D) is_array_cl = true;
    }
    
    if(!is_colon) return false;

    uint8_t is_quo_mk = false;
    // Find quotation mark
    for(l_index = strlen(line) - 1; l_index >= 0; l_index--) {
        if
        (
            line[l_index] == 0x22 || 
            line[l_index] == 0x27
        ) is_quo_mk = true;

        else if(!is_quo_mk && line[l_index] == 0x7D) {
            is_array_cl = true;
            break;
        }
    }

    // Check for parsing error
    if(is_array_beg && !is_array_cl) {
        PERR("unclosed array", line_index + 1);
        exit(ERRC_PARSE);
    }

    else if(is_array_beg && is_array_cl)
        (*p_kpi)[(*p_kpi_c) - 1].is_arr_elem = true;

    else 
        (*p_kpi)[(*p_kpi_c) - 1].is_arr_elem = false;

    // Find the minimum points
    for(l_index = 0; l_index < (*p_kpi_c); l_index++) {
        for(r_index = (*p_kpi)[l_index].max_index; r_index >= 0; r_index--) {
            // Check for the breakpoint characters
            if
            (
                line[r_index] == 0x7B || 
                line[r_index] == 0x2C
            ) {
                (*p_kpi)[l_index].min_index = r_index + 1;
                (*p_kpi)[l_index].is_arr_elem = true;
                found_min = true;
                break;
            }
        }

        if(!found_min) (*p_kpi)[l_index].min_index = 0;
        found_min = false;
    }

    return true;
}


/* Extract all values from yaml list */
static void yamlExtractList(char *list, KeyData *p_key) {
    int32_t l_index, ch_index;
    char *tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    // Populate ValueData instance
    for(l_index = 1, ch_index = 0; l_index < strlen(list); l_index++) {
        if(list[l_index] == 0x2C || list[l_index] == 0x5D) {
            trimStr(&tmp_str, TRIM_BOTH_SIDES);
            yamlPushKeyValue(tmp_str, p_key);
            ch_index = 0;
        }      

        else {
            tmp_str[ch_index] = list[l_index];
            ch_index++;            
        }
    }

    free(tmp_str);
}


/* Get all values from key line */
static void yamlGetKeyLineValues(char *line, KeyData *p_key, char *p_op, uint8_t *p_is_op) {
    int32_t l_index, ch_index;
    char *tmp_str = (char*) calloc (
        YAML_CH_BUFFER_SIZE,
        sizeof(char)
    );
    
    // Populate tmp_str with data
    for
    (
        l_index = p_key->colon_index + 2, ch_index = 0; 
        l_index < strlen(line); 
        l_index++, ch_index++
    ) {
        // Check if string needs more memory 
        if(ch_index >= YAML_CH_BUFFER_SIZE) {
            char *p_tmp = (char*) realloc (
                tmp_str, 
                sizeof(char) * (ch_index + 1)
            );

            if(!p_tmp) {
                MEMERR("failed to allocate memory for tmp_str");
                exit(ERRC_MEM);
            }
            tmp_str = p_tmp;
        }

        tmp_str[ch_index] = line[l_index];
    }
    
    trimStr (
        &tmp_str, 
        TRIM_BOTH_SIDES
    );

    // Check for operators
    if
    (
        tmp_str[0] == '>' || 
        tmp_str[0] == '|'
    ) {
        *p_op = tmp_str[0];
        *p_is_op = true;
    }

    // Check if list is present
    else if
    (
        tmp_str[0] == 0x5B && 
        tmp_str[strlen(tmp_str) - 1] == 0x5D
    ) {
        yamlExtractList (
            tmp_str, 
            p_key
        );
    }

    // Check if value is key-value array element
    else if(p_key->is_sub_key) {
        // Find possible breakpoints which are 0x2C and 0x7D 
        for(l_index = 0; l_index < strlen(tmp_str); l_index++)
            if
            (
                tmp_str[l_index] == 0x2C ||
                tmp_str[l_index] == 0x7D
            ) break;
        
        cropStr (
            &tmp_str, 
            TRIM_END, 
            strlen(tmp_str) - l_index + 1
        );

        yamlPushKeyValue(tmp_str, p_key);
    }

    // Standard value
    else {
        trimStr(&tmp_str, TRIM_BOTH_SIDES);
        yamlPushKeyValue(tmp_str, p_key);
    }

    free(tmp_str);
    ch_index = 0;
}


static uint8_t yamlGetGenLineValues(char *line, KeyData *p_key, int32_t min_ws, int32_t max_ws, char op, uint8_t is_op) {
    uint8_t is_list = false;
    int32_t l_index, ch_index, l_ws_c;
    
    char *tmp_str = (char*) calloc (
        YAML_CH_BUFFER_SIZE,
        sizeof(char)
    );

    // Find the whitespace amount on the line elements
    for(l_ws_c = 0; l_ws_c < strlen(line); l_ws_c++)
        if(line[l_ws_c] != 0x20) break;

    // Check if key is array element
    if(p_key->is_sub_key) return false;

    // Detected operator
    if
    (
        is_op && 
        l_ws_c > min_ws 
    ) {

        switch (op)
        {
        // Skip all newlines
        case '>':
            // Populate tmp_str while skipping all newlines
            for(l_index = l_ws_c, ch_index = 0; l_index < strlen(line); l_index++) {
                if(line[l_index] != 0x0A && line[l_index] != 0x0D) {
                    // Check if more memory is needed for the
                    if(ch_index >= YAML_CH_BUFFER_SIZE) {
                        char *p_tmp = (char*) realloc (
                            tmp_str,
                            sizeof(char) * (ch_index + 1)
                        );

                        if(!p_tmp) {
                            MEMERR("failed to reallocate tmp_str");
                            exit(ERRC_MEM);
                        }

                        tmp_str = p_tmp;
                    }

                    tmp_str[ch_index] = line[l_index];
                    ch_index++;
                }
            }
            break;
        
        // Preserve all newlines
        case '|':
            for(l_index = l_ws_c; l_index < strlen(line); l_index++) {
                if(ch_index >= YAML_CH_BUFFER_SIZE) {
                    char *p_tmp = (char*) realloc (
                        tmp_str,
                        sizeof(char) * (ch_index + 1)
                    );

                    if(!p_tmp) {
                        MEMERR("failed to reallocate tmp_str");
                        exit(ERRC_MEM);
                    }

                    tmp_str = p_tmp;
                }

                tmp_str[ch_index] = line[l_index];
                ch_index++;
            }
            break;
        
        default:
            break;
        }
    }

    // Key content specified with hyphen 0x2D
    else if 
    (
        !is_op&& 
        l_ws_c > min_ws &&
        (max_ws != -1 ? l_ws_c <= max_ws : true) &&
        line[l_ws_c] == 0x2D
    ) {
        // Skip all whitespaces
        for(l_index = l_ws_c + 1; l_index < strlen(line); l_index++)
            if(line[l_index] != 0x20) break;

        // Read values into tmp_str
        for(ch_index = 0; l_index < strlen(line); l_index++, ch_index++) {
            // Check if more memory needs to be allocated for tmp_str
            if(ch_index >= YAML_CH_BUFFER_SIZE) {
                char *p_tmp = (char*) realloc (
                    tmp_str,
                    sizeof(char) * (ch_index + 1)
                );

                if(!p_tmp) {
                    MEMERR("failed to reallocate tmp_str");
                    exit(ERRC_MEM);
                }

                tmp_str = p_tmp;
            }
            tmp_str[ch_index] = line[l_index];
        }

        trimStr(&tmp_str, TRIM_BOTH_SIDES);

        // Detected a list
        if
        (
            tmp_str[0] == 0x5B && 
            tmp_str[strlen(tmp_str) - 1] == 0x5D
        ) {
            yamlExtractList(tmp_str, p_key), 
            is_list = true;
        }
    }

    else if
    (
        l_ws_c <= min_ws ||
        p_key->is_sub_key
    ) {
        free(tmp_str);
        return false;
    }

    // Copy data from tmp_str to val_str
    if 
    (
        !is_list &&
        (max_ws != -1 ? l_ws_c <= max_ws : true) &&
        line[l_ws_c] == 0x2D
    ) yamlPushKeyValue(tmp_str, p_key);

    free(tmp_str);
    return true;
}


/* Push value to KeyData instance */
static void yamlPushKeyValue(char *val, KeyData *p_key) {
    // Check for quotation in the beginning
    char *tmp_str = (char*) calloc (
        strlen(val) + 1,
        sizeof(char)
    );

    strncpy (
        tmp_str,
        val,
        strlen(val)
    );

    char mk = 0x00;
    if
    (
        val[0] == 0x22 ||
        val[0] == 0x27 
    ) mk = val[0];

    // Check for quotation marks in the end
    if(val[strlen(val) - 1] == mk) 
        cropStr(&tmp_str, TRIM_BOTH_SIDES, 1);
    
    p_key->key_val_c++;
    char **pp_tmp = (char**) realloc (
        p_key->key_vals,
        sizeof(char*) * p_key->key_val_c
    );

    if(!pp_tmp) {
        MEMERR("Failed to reallocate memory for key values");
        exit(ERRC_MEM);
    }

    p_key->key_vals = pp_tmp;
    p_key->key_vals[p_key->key_val_c - 1] = (char*) calloc (
        strlen(tmp_str) + 1,
        sizeof(char)
    );

    strncpy (
        p_key->key_vals[p_key->key_val_c - 1],
        tmp_str,
        strlen(tmp_str)
    );

    free(tmp_str);
}


/* Find all key names in one line */
static void yamlFindKeys(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyData **p_key_data, int32_t *p_key_c) {
    int32_t l_index, r_index, ch_index;
    KeyPosInfo *kpi = (KeyPosInfo*) calloc(1, sizeof(KeyPosInfo));
    int32_t kpi_c = 0;

    int32_t l_ws_c;
    char *tmp_str = calloc(YAML_CH_BUFFER_SIZE, 1);

    // Check if key name bounds are available 
    if
    (
        !yamlFindKeyNameBounds (
            line, 
            line_index, 
            min_index, 
            max_index, 
            &kpi, 
            &kpi_c
        )
    ) return;

    for(l_index = 0; l_index < kpi_c; l_index++) {
        ch_index++;
        // Save the name data into tmp_str
        for(r_index = kpi[l_index].min_index, ch_index = 0; r_index < kpi[l_index].max_index; r_index++, ch_index++)
            tmp_str[ch_index] = line[r_index];
        
        // Find the total amount of whitespaces from beginning of line to key name instance
        if(kpi[l_index].is_arr_elem) l_ws_c = -1;
        else {
            for(l_ws_c = 0; l_ws_c < strlen(line); l_ws_c++) 
                if(line[l_ws_c] != 0x20) break;
        }

        // Check if key with operator is specified previously
        if(op_key_ws_c != -1 && l_ws_c > op_key_ws_c) return;
        else if(op_key_ws_c != -1) op_key_ws_c = -1; 

        // Check for any operators on current key line
        for(r_index = kpi[l_index].max_index + 1; r_index < strlen(line); r_index++) {
            if(line[r_index] == '>' || line[r_index] == '|') {
                op_key_ws_c = l_ws_c;
                break;
            }
        }

        // Reallocate and populate memory for new KeyData instance
        (*p_key_c)++;
        
        KeyData *p_tmp = (KeyData*) realloc (
            (*p_key_data), 
            (*p_key_c) * sizeof(KeyData)
        );

        if(!p_tmp) {
            MEMERR("failed to reallocate memory for key data");
            exit(ERRC_MEM);
        }

        (*p_key_data) = p_tmp;
        (*p_key_data)[(*p_key_c) - 1].key_val_c = 0;
        (*p_key_data)[(*p_key_c) - 1].key_vals = (char**) calloc(1, sizeof(char*));
        (*p_key_data)[(*p_key_c) - 1].line = line_index;
        (*p_key_data)[(*p_key_c) - 1].colon_index = kpi[l_index].max_index; 
        (*p_key_data)[(*p_key_c) - 1].is_sub_key = kpi[l_index].is_arr_elem;
        (*p_key_data)[(*p_key_c) - 1].ws_c = l_ws_c;
        trimStr(&tmp_str, TRIM_BOTH_SIDES);
        
        // Check if key name is string
        if(tmp_str[0] == 0x22 || tmp_str[0] == 0x27)
            cropStr(&tmp_str, TRIM_BOTH_SIDES, 1);
        
        (*p_key_data)[(*p_key_c) - 1].key_name = (char*) calloc(strlen(tmp_str) + 1, 1);
        strncpy((*p_key_data)[(*p_key_c) - 1].key_name, tmp_str, strlen(tmp_str));
        
        free(tmp_str);
        tmp_str = (char*) calloc (
            YAML_CH_BUFFER_SIZE,
            sizeof(char)
        );
    }

    free(tmp_str);
    free(kpi);
}



/* Key name parsing function */
static void yamlParseKeys(char **lines, int32_t line_c, int32_t *p_li_index, KeyData **p_key_data, int32_t *p_key_c) {
    int32_t index;
    StrBounds *str_bds = NULL;
    int32_t str_bds_c = 0;
    uint8_t is_str = yamlSearchString (
        lines[(*p_li_index)], 
        (*p_li_index), 
        &str_bds, 
        &str_bds_c
    );

    if(is_str) {
        for(index = 0; index < str_bds_c; index++) {
            if(!index) 
                yamlFindKeys (
                    lines[(*p_li_index)], 
                    (*p_li_index), 
                    0, 
                    str_bds[index].str_beg, 
                    p_key_data, 
                    p_key_c
                );
            
            if(index == str_bds_c - 1) 
                yamlFindKeys (
                    lines[(*p_li_index)], 
                    (*p_li_index), 
                    str_bds[index].str_end + 1, 
                    strlen(lines[(*p_li_index)]), 
                    p_key_data, 
                    p_key_c
                );
            
            else 
                yamlFindKeys (
                    lines[(*p_li_index)], 
                    (*p_li_index), 
                    str_bds[index - 1].str_end + 1, 
                    str_bds[index].str_beg, 
                    p_key_data, 
                    p_key_c
                );
        }
    }
    else 
        yamlFindKeys (
            lines[(*p_li_index)], 
            (*p_li_index), 
            0, 
            strlen(lines[(*p_li_index)]), 
            p_key_data, 
            p_key_c
        );

    if(str_bds) free(str_bds);
}


/* Key value parsing function */ 
static void yamlParseValues(char **lines, int32_t line_c, KeyData *keys, int32_t key_c, int32_t key_index) {
    int32_t l_index;
    uint8_t is_op = false;
    char operator;

    // Check if the key is not the last key
    int32_t min_ws = keys[key_index].ws_c;
    int32_t max_ws = -1;
    if
    (
        key_index != key_c - 1 &&
        keys[key_index].ws_c != -1
    ) {

        // Check for the whitespace count of following key values
        for(l_index = key_index + 1; l_index < key_c; l_index++) {
            if
            (
                keys[l_index].ws_c > min_ws && 
                (max_ws == -1 || max_ws > keys[l_index].ws_c) &&
                !keys[l_index].is_sub_key
            )
                max_ws = keys[l_index].ws_c;

            else if 
            (
                keys[l_index].ws_c >= 0 &&
                keys[l_index].ws_c <= min_ws 
            ) break;
        }
    }

    // Check all lines following the
    for(l_index = keys[key_index].line; l_index < line_c; l_index++) {
        // Check if current line is the first line
        if
        (
            l_index == keys[key_index].line && 
            keys[key_index].colon_index + 2 < strlen(lines[l_index])
        ) {
            yamlGetKeyLineValues(lines[l_index], &keys[key_index], &operator, &is_op);
            if(is_op) {
                multi_line_str = calloc (
                    YAML_CH_BUFFER_SIZE,
                    sizeof(char)
                );
                ml_str_index = 0;   
            }
        }
        
        // Line is not first line
        else if
        (   
            l_index != keys[key_index].line &&
            !yamlGetGenLineValues (
                lines[l_index], 
                &keys[key_index], 
                min_ws, 
                max_ws, 
                operator, 
                is_op
            )
        ) return;
    }
}


/* Remove all comments from the lines */
static void yamlRemoveEmptyLines(char ***p_lines, int32_t *p_size) {
    int32_t l_index, r_index;
    char **cpy_lines = (char**) calloc(1, sizeof(char*));
    int32_t cpy_lines_c = 0;

    // Iterate throuh every line and check if it isn't empty
    uint8_t found_content = false;
    for(l_index = 0; l_index < (*p_size); l_index++) {
        for(r_index = 0; r_index < strlen((*p_lines)[l_index]); r_index++) {
            if((*p_lines)[l_index][r_index] != 0x20) {
                found_content = true;
                break;
            }
        }

        if(found_content && strlen((*p_lines)[l_index])) {
            cpy_lines_c++;
            char **pp_tmp = (char**) realloc (
                cpy_lines, 
                cpy_lines_c * sizeof(char*)
            );

            if(!pp_tmp) {
                MEMERR("failed to allocate memory for cpy_lines");
                exit(ERRC_MEM);
            }

            cpy_lines = pp_tmp;
            cpy_lines[cpy_lines_c - 1] = (char*) calloc (
                strlen((*p_lines)[l_index]) + 1, 
                1
            );

            strncpy (
                cpy_lines[cpy_lines_c - 1], 
                (*p_lines)[l_index], 
                strlen((*p_lines)[l_index]) + 1
            );
            found_content = false;
        }
    }

    // Clean the current line information
    for(l_index = 0; l_index < (*p_size); l_index++)
        free((*p_lines)[l_index]);    
    free((*p_lines));

    (*p_lines) = cpy_lines;
    (*p_size) = cpy_lines_c;
}


/* Remove all comments from all lines */
static void yamlRemoveComments(char **lines, int32_t lines_c) {
    int32_t l_index, r_index;
    char *tmp_str;

    // Check for comments on every line
    uint8_t found_comment = false;
    for(l_index = 0; l_index < lines_c; l_index++) {
        for(r_index = 0; r_index < strlen(lines[l_index]); r_index++) {
            if(lines[l_index][r_index] == 0x23) {
                found_comment = true;
                break;
            }
        }
        
        // Redo the string so it is uncommented
        if(found_comment) {
            tmp_str = (char*) calloc (
                r_index + 1, 
                1
            );

            strncpy (
                tmp_str, 
                lines[l_index], 
                r_index
            );

            tmp_str[r_index] = 0x00;

            free(lines[l_index]);
            lines[l_index] = (char*) calloc (
                r_index + 1, 
                1
            );
            
            strncpy (
                lines[l_index], 
                tmp_str, 
                strlen(tmp_str)
            );

            found_comment = false;
            free(tmp_str);
        }
    }
}


// Sort the file data by lines
static void yamlLines(char ***p_lines, int32_t *p_size, char *file_contents) {
    int32_t l_index, r_index, ch_index;
    (*p_size) = 0;

    // Count all new lines
    for(l_index = 0; l_index < strlen(file_contents); l_index++)
        if(file_contents[l_index] == 0x0A) (*p_size)++;

    // Allocate memory for lines
    (*p_size)++;
    (*p_lines) = (char**) calloc(
        (*p_size), 
        sizeof(char*)
    );
    for(l_index = 0; l_index < (*p_size); l_index++)
        (*p_lines)[l_index] = (char*) calloc(YAML_CH_BUFFER_SIZE, 1);

    // Populate lines data
    r_index = 0; 
    ch_index = 0;
    for(l_index = 0; l_index < strlen(file_contents); l_index++) {
        if(file_contents[l_index] != 0x0A && file_contents[l_index] != 0x0D) {
            (*p_lines)[r_index][ch_index] = file_contents[l_index];
            ch_index++;
        }

        else if(file_contents[l_index] == 0x0A){
            ch_index = 0; 
            r_index++;
        }
    }
}


/* Main parsing callback function */
void yamlParse(KeyData **p_key_data, int32_t *p_key_c, char *file_name) {
    int32_t l_index;
    FILE *file;
    file = fopen(file_name, "rb");

    // Check for file error
    if(!file) {
        printf("Failed to open file: %s\n", file_name);
        exit(ERRC_FILE);
    }

    int res;
    // Get the file size
    int32_t file_size;
    res = fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    res = fseek(file, 0, SEEK_SET);
    if(res) printf("Failed to find file size for file %s\n", file_name);
    
    // Read all file data into contents variable
    char *file_contents = (char*) calloc(file_size + 1, 1);
    res = fread(file_contents, 1, file_size, file);
    
    char **lines;
    int32_t line_c;

    KeyData *key_data = (KeyData*) calloc(1, sizeof(KeyData));
    int32_t key_c = 0;

    yamlLines (
        &lines, 
        &line_c, 
        file_contents
    );

    yamlRemoveComments (
        lines, 
        line_c
    );

    yamlRemoveEmptyLines (
        &lines, 
        &line_c
    );

    for(l_index = 0; l_index < line_c; l_index++) 
        yamlParseKeys (
            lines, 
            line_c, 
            &l_index, 
            &key_data, 
            &key_c
        );

    for(l_index = 0; l_index < key_c; l_index++)
        yamlParseValues (
            lines,
            line_c,
            key_data,
            key_c,
            l_index
        );

    (*p_key_data) = key_data;
    (*p_key_c) = key_c;
}