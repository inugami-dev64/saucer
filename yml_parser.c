#define YAML_PARSER_PRIVATE
#include "saucer.h"

/* Generic yaml parser helpers */
/* Check if line contains string */
static uint8_t yamlSearchString(char *line, int32_t line_index, StrBounds **p_str_bds, int32_t *p_str_bds_c) {
    int32_t index;
    CharInfo *qms = (CharInfo*) calloc(1, sizeof(CharInfo));
    int32_t qm_c = 0;
    uint8_t is_str_found = false;
    
    // Find the minimum quotation mark index
    for(index = 0; index < strlen(line); index++) {
        if(line[index] == 0x22 || line[index] == 0x27) {
            qm_c++;
            qms = (CharInfo*) realloc(qms, qm_c * sizeof(CharInfo));
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
        exit(-1);
    }

    if(d_quo_c % 2) {
        PERR("unclosed double quotation mark string", line_index + 1);
        exit(-1);
    }

    for(index = 0; index < qm_c - 1; index += 2) {
        // Check for parsing error
        if(qms[index].ch != qms[index + 1].ch) {
            PERR("quotation marks (0x27 and 0x22) are not allowed to be used inside strings", line_index + 1);
            exit(-1);
        }

        (*p_str_bds_c)++;
        (*p_str_bds) = (StrBounds*) realloc((*p_str_bds), (*p_str_bds_c) * sizeof(StrBounds));
        (*p_str_bds)[(*p_str_bds_c) - 1].str_beg = qms[index].index;
        (*p_str_bds)[(*p_str_bds_c) - 1].str_end = qms[index + 1].index;
    }

    return is_str_found;
}


/* Find the key name colon positions and name beginning breakpoint position on line */
static void yamlFindKeyNameBounds(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyPosInfo **p_kpi, int32_t *p_kpi_c) {
    int32_t l_index, r_index;
    uint8_t found_min = false;
    uint8_t is_array_beg = false;
    uint8_t is_array_cl = false;

    // Search for colons
    for(l_index = min_index; l_index < max_index; l_index++) {
        if(line[l_index] == 0x3A) {
            (*p_kpi_c)++;
            (*p_kpi) = (KeyPosInfo*) realloc((*p_kpi), (*p_kpi_c) * sizeof(KeyPosInfo));
            (*p_kpi)[(*p_kpi_c) - 1].max_index = l_index;

            // Check for parsing error
            if(l_index + 1 < max_index && line[l_index + 1] != 0x20) {
                PERR("expected whitespace after key name declaration", line_index + 1);
                exit(-1);
            }
        }        

        if(line[l_index] == 0x7B) is_array_beg = true;
        if(line[l_index] == 0x7D) is_array_cl = true;
    }

    for(l_index = strlen(line) - 1; l_index >= 0; l_index--) {
        if(line[l_index] == 0x7D) {
            is_array_cl = true;
            break;
        }
    }

    // Check for parsing error
    if(is_array_beg && !is_array_cl) {
        PERR("unclosed array", line_index + 1);
        exit(-1);
    }

    // Find the minimum points
    for(l_index = 0; l_index < (*p_kpi_c); l_index++) {
        for(r_index = (*p_kpi)[l_index].max_index; r_index >= 0; r_index--) {
            // Check for the breakpoint characters
            if(line[r_index] == 0x7B || line[r_index] == 0x2C) {
                (*p_kpi)[l_index].min_index = r_index + 1;
                (*p_kpi)[l_index].is_arr_elem = true;
                found_min = true;
                break;
            }
        }

        if(!found_min) (*p_kpi)[l_index].min_index = 0;
        found_min = false;
    }
}


/* Extract all values from yaml list */
static void yamlExtractList(char *list, int32_t line_index, int32_t ws_c, int32_t list_index, ValueData **p_val_data, int32_t *p_val_c) {
    int32_t l_index, ch_index;
    char *tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    // Allocate memory for contents
    (*p_val_c)++;
    (*p_val_data) = (ValueData*) realloc((*p_val_data), sizeof(ValueData) * (*p_val_c));

    // Populate ValueData instance
    for(l_index = 1, ch_index = 0; l_index < strlen(list); l_index++) {
        if(list[l_index] == 0x2C || list[l_index] == 0x5D) {
            (*p_val_data)[(*p_val_c) - 1].line = line_index;
            (*p_val_data)[(*p_val_c) - 1].start_index = list_index + l_index;
            (*p_val_data)[(*p_val_c) - 1].ws_c = ws_c;

            trimStr(&tmp_str, TRIM_BOTH_SIDES);
            (*p_val_data)[(*p_val_c) - 1].value_str = (char*) calloc(strlen(tmp_str) + 1, sizeof(char));
            strncpy((*p_val_data)[(*p_val_c) - 1].value_str, tmp_str, strlen(tmp_str));
            
            // Clean tmp str
            ch_index = 0;
            free(tmp_str);
            tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

            // Allocate more memory for ValueData if needed
            if(list[l_index] != 0x5D) {
                (*p_val_c)++;
                (*p_val_data) = (ValueData*) realloc((*p_val_data), (*p_val_c) * sizeof(ValueData));    
            }
        }      

        else {
            tmp_str[ch_index] = list[l_index];
            ch_index++;            
        }
    }
}


/* Find values that are directly next to the key declaration */
static void yamlFindDirectValues(char *line, int32_t line_index, int32_t *kci, int32_t kci_c, ValueData **p_val_data, int32_t *p_val_c) {
    uint8_t is_content = false;
    int32_t l_index, r_index, ch_index;
    int32_t val_s_index = 0;
    char *tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    for(l_index = 0; l_index < kci_c; l_index++) {
        for(r_index = kci[l_index] + 1; r_index < strlen(line); r_index++) {
            if(line[r_index] != 0x20) {
                is_content = true;
                break;
            }
        }

        // Check if key name declaration contains value
        if(!is_content) return;

        // Record value data to tmp_str
        for(r_index = kci[l_index] + 1, ch_index = 0; r_index < strlen(line); r_index++, ch_index++)
            tmp_str[ch_index] = line[r_index];

        // Trim tmp_str
        trimStr(&tmp_str, TRIM_BOTH_SIDES);

        if(tmp_str[0] == 0x5B) {
            uint8_t is_cl = false;
            for(r_index = 1; r_index < strlen(tmp_str); r_index++) {
                if(tmp_str[r_index] == 0x5D) {
                    is_cl = true;
                    break;
                } 
            }
            // Check for parsing error
            if(!is_cl) {
                PERR("unclosed list", line_index + 1);
                exit(-1);
            }

            // Find the starting index of the list
            for(val_s_index = kci[l_index] + 1; val_s_index < strlen(line); val_s_index++)
                if(line[val_s_index] == 0x5B) break;

            // Remove all irrelevant data
            for(r_index++; r_index < strlen(tmp_str); r_index++)
                tmp_str[r_index] = 0x00;

            yamlExtractList(tmp_str, line_index, -1, val_s_index, p_val_data, p_val_c);
            return;
        }

        // Found quotation marks now search for the ending mark
        else if(tmp_str[0] == 0x22 || tmp_str[0] == 0x27) {
            // Find same type of quotation marks
            char mk = tmp_str[0];
            for(r_index = 1; r_index < strlen(tmp_str); r_index++)
                if(tmp_str[r_index] == mk) break;

            // Remove irrelevant data
            for(r_index++; r_index < strlen(tmp_str); r_index++)
                tmp_str[r_index] = 0x00;

            // Find the start index
            for(val_s_index = kci[l_index] + 1; val_s_index < strlen(line); val_s_index++)
                if(line[val_s_index] == mk) break;
            
            val_s_index++;
            cropStr(&tmp_str, TRIM_BOTH_SIDES, 1);
        }

        else if(tmp_str[0] != 0x7B) {
            // Find the breakpoint
            for(r_index = 0; r_index < strlen(tmp_str); r_index++) 
                if(tmp_str[r_index] == 0x2C || tmp_str[r_index] == 0x5D) break;
            
            // Remove irrelevant data
            for(; r_index < strlen(tmp_str); r_index++)
                tmp_str[r_index] = 0x00;

            // Find the start index
            for(val_s_index = kci[l_index] + 1; val_s_index < strlen(line); val_s_index++)
                if(line[val_s_index] != 0x20) break;

            trimStr(&tmp_str, TRIM_BOTH_SIDES);
        }

        if(tmp_str) {
            (*p_val_c)++;
            printf("Count: %d\n", (*p_val_c));
            (*p_val_data) = (ValueData*) realloc((*p_val_data), (*p_val_c) * sizeof(ValueData));
            (*p_val_data)[(*p_val_c) - 1].line = line_index;
            (*p_val_data)[(*p_val_c) - 1].start_index = val_s_index;
            (*p_val_data)[(*p_val_c) - 1].ws_c = -1;
            (*p_val_data)[(*p_val_c) - 1].value_str = (char*) calloc(strlen(tmp_str) + 1, sizeof(char));
            strncpy((*p_val_data)[(*p_val_c) - 1].value_str, tmp_str, strlen(tmp_str));
            free(tmp_str);
        }
    }
}


static void yamlFindHyphenValues(char *line, int32_t line_index, int32_t hyph_index, ValueData **p_val_data, int32_t *p_val_c) {
    int32_t l_index, ch_index;
    int32_t c_index;
    char *tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    // Check for parsing error
    if(hyph_index + 1 < strlen(line) && line[hyph_index + 1] != 0x20) {
        PERR("expected whitespace after hyphen (0x2D)", line_index + 1);
        exit(-1);
    }

    // Populate tmp_str with values data
    for(l_index = hyph_index + 1, ch_index = 0; l_index < strlen(line); l_index++, ch_index++)
        tmp_str[ch_index] = line[l_index];

    trimStr(&tmp_str, TRIM_BOTH_SIDES);

    // Find the value starting index
    for(c_index = hyph_index + 1; c_index < strlen(line); c_index++)
        if(line[c_index] != 0x20) break;
    
    // Check if data is in list
    if(tmp_str[0] == 0x5B) {
        // Check if list is closed
        uint8_t is_cl = false;
        for(l_index = 1; l_index < strlen(tmp_str); l_index++) {
            if(tmp_str[l_index] == 0x5D) {
                is_cl = true;
                break;
            }
        }

        if(!is_cl) {
            PERR("unclosed list", line_index + 1);
            exit(-1);
        }

        yamlExtractList(tmp_str, line_index, hyph_index, c_index, p_val_data, p_val_c);
        return;
    }

    // Check for quotation marks and if needed trim the string
    char mk;
    if(tmp_str[0] == 0x22 || tmp_str[0] == 0x27) {
        mk = tmp_str[0];
        uint8_t is_quo_cl = false;
        for(l_index = 1; l_index < strlen(tmp_str); l_index++) {
            if(tmp_str[l_index] == mk) {
                is_quo_cl = true;
                break;
            }
        }

        // Check for parsing error
        if(!is_quo_cl) {
            PERR("unclosed string", line_index + 1);
            exit(-1);
        }
        
        cropStr(&tmp_str, TRIM_END, (int) (strlen(tmp_str) - 1 - l_index));
        cropStr(&tmp_str, TRIM_FRONT, 1);
        printf("STR: %s\n", tmp_str);
    }

    (*p_val_c)++;
    printf("value size, c is: %d, %d, %s\n", (*p_val_c) * sizeof(ValueData), (*p_val_c), tmp_str);
    (*p_val_data) = (ValueData*) realloc((*p_val_data), (*p_val_c) * sizeof(ValueData));
    printf("seg test2\n");
    (*p_val_data)[(*p_val_c) - 1].line = line_index;
    (*p_val_data)[(*p_val_c) - 1].start_index = c_index;
    (*p_val_data)[(*p_val_c) - 1].ws_c = hyph_index;
    (*p_val_data)[(*p_val_c) - 1].value_str = calloc(strlen(tmp_str) + 1, sizeof(char));
    strncpy((*p_val_data)[(*p_val_c) - 1].value_str, tmp_str, strlen(tmp_str));
    free(tmp_str);
}


// /* Find all values specified */
static void yamlFindValues(char *line, int32_t line_index, ValueData **p_val_data, int32_t *p_val_c, KeyData *keys, int32_t key_c) {
    int32_t l_index;
    int32_t *kci = (int32_t*) calloc(1, sizeof(int32_t));
    int32_t kci_c = 0;

    uint8_t is_hyphen = false;
    int32_t hyph_index;
    // Check if keys are present on current line 
    for(l_index = 0; l_index < key_c; l_index++) {
        if(keys[l_index].line == line_index) {
            kci_c++;
            kci = (int32_t*) realloc(kci, sizeof(int32_t) * kci_c);
            kci[kci_c - 1] = keys[l_index].colon_index;
        }

        else if(keys[l_index].line > line_index) break;
    }

    // Search for hyphen
    for(l_index = 0; l_index < strlen(line); l_index++) {
        if(line[l_index] == 0x2D) {
            is_hyphen = true;
            hyph_index = l_index;
        }

        else if(line[l_index] != 0x20) break;
    }

    // Keys are present on the current line, so check for their values
    if(kci_c)
        yamlFindDirectValues(line, line_index, kci, kci_c, p_val_data, p_val_c);

    if(is_hyphen) 
        yamlFindHyphenValues(line, line_index, hyph_index, p_val_data, p_val_c);
}


/* Find all key names in one line */
static void yamlFindKeys(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyData **p_key_data, int32_t *p_key_c) {
    int32_t l_index, r_index, ch_index;
    KeyPosInfo *kpi = (KeyPosInfo*) calloc(1, sizeof(KeyPosInfo));
    int32_t kpi_c = 0;

    int32_t l_ws_c;
    char *tmp_str = calloc(YAML_CH_BUFFER_SIZE, sizeof(char));
    yamlFindKeyNameBounds(line, line_index, min_index, max_index, &kpi, &kpi_c);

    for(l_index = 0; l_index < kpi_c; l_index++) {
        for(r_index = kpi[l_index].min_index, ch_index = 0; r_index < kpi[l_index].max_index; r_index++, ch_index++)
            tmp_str[ch_index] = line[r_index];
        
        // Find the total amount of whitespaces from beginning of line to key name instance
        if(kpi[l_index].is_arr_elem) l_ws_c = -1;
        else {
            for(l_ws_c = 0; l_ws_c < strlen(line); l_ws_c++) 
                if(line[l_ws_c] != 0x20) break;
        }

        // Reallocate and populate memory for new KeyData instance
        (*p_key_c)++;
        (*p_key_data) = (KeyData*) realloc((*p_key_data), (*p_key_c) * sizeof(KeyData));
        (*p_key_data)[(*p_key_c) - 1].key_val_c = 0;
        (*p_key_data)[(*p_key_c) - 1].key_vals = (char*) calloc(1, sizeof(char));
        (*p_key_data)[(*p_key_c) - 1].line = line_index;
        (*p_key_data)[(*p_key_c) - 1].colon_index = kpi[l_index].max_index; 
        (*p_key_data)[(*p_key_c) - 1].ws_c = l_ws_c;
        trimStr(&tmp_str, TRIM_BOTH_SIDES);
        (*p_key_data)[(*p_key_c) - 1].key_name = (char*) calloc(strlen(tmp_str) + 1, sizeof(char));
        strncpy((*p_key_data)[(*p_key_c) - 1].key_name, tmp_str, strlen(tmp_str));
        
        free(tmp_str);
        tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));
    }

    free(tmp_str);
}


/* Main parsing function */
static void yamlParseLine(char *line, int32_t line_index, KeyData **p_key_data, int32_t *p_key_c, ValueData **p_val_data, int32_t *p_val_c) {
    int32_t index;
    StrBounds *str_bds = NULL;
    int32_t str_bds_c = 0;
    uint8_t is_str = yamlSearchString(line, line_index, &str_bds, &str_bds_c);

    if(is_str) {
        for(index = 0; index < str_bds_c; index++) {
            if(!index) yamlFindKeys(line, line_index, 0, str_bds[index].str_beg, p_key_data, p_key_c);
            else if(index == str_bds_c - 1) yamlFindKeys(line, line_index, str_bds[index].str_end + 1, strlen(line), p_key_data, p_key_c);
            else yamlFindKeys(line, line_index, str_bds[index - 1].str_end + 1, str_bds[index].str_beg, p_key_data, p_key_c);
        }
    }
    else yamlFindKeys(line, line_index, 0, strlen(line), p_key_data, p_key_c);

    yamlFindValues(line, line_index, p_val_data, p_val_c, *p_key_data, *p_key_c);
    if(str_bds) free(str_bds);
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
            cpy_lines = (char**) realloc((void*) cpy_lines, cpy_lines_c * sizeof(char*));
            cpy_lines[cpy_lines_c - 1] = (char*) calloc(strlen((*p_lines)[l_index]) + 1, sizeof(char));
            strncpy(cpy_lines[cpy_lines_c - 1], (*p_lines)[l_index], strlen((*p_lines)[l_index]));
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
        if(found_comment && r_index) {
            tmp_str = (char*) calloc(r_index + 1, sizeof(char));
            strncpy(tmp_str, lines[l_index], r_index);
            tmp_str[r_index] = 0x00;

            free(lines[l_index]);
            lines[l_index] = (char*) calloc(r_index + 1, sizeof(char));
            strncpy(lines[l_index], tmp_str, strlen(tmp_str));

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
    (*p_lines) = (char**) calloc((*p_size), sizeof(char*));
    for(l_index = 0; l_index < (*p_size); l_index++)
        (*p_lines)[l_index] = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    // Populate lines data
    r_index = 0; 
    ch_index = 0;
    for(l_index = 0; l_index < strlen(file_contents); l_index++) {
        if(file_contents[l_index] != 0x0A) {
            (*p_lines)[r_index][ch_index] = file_contents[l_index];
            ch_index++;
        }

        else {
            ch_index = 0; 
            r_index++;
        }
    }
}


/* Main parsing callback function */
void yamlParse(const char *file_name) {
    int32_t index;
    FILE *file;
    file = fopen(file_name, "rb");

    // Check for file error
    if(!file) {
        printf("Failed to open file: %s\n", file_name);
        exit(-2);
    }

    int res;
    // Get the file size
    int32_t file_size;
    res = fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    res = fseek(file, 0, SEEK_SET);
    if(res) printf("Failed to find file size for file %s\n", file_name);

    // Read all file data into contents variable
    char *file_contents = (char*) calloc(file_size + 1, sizeof(char));
    res = fread(file_contents, sizeof(char), file_size, file);
    file_contents[file_size] = 0x00;
    
    char **lines;
    int32_t line_c;

    KeyData *key_data = (KeyData*) calloc(1, sizeof(KeyData));
    int32_t key_c = 0;

    ValueData *val_data = (ValueData*) calloc(1, sizeof(ValueData));
    int32_t val_c = 0;

    yamlLines(&lines, &line_c, file_contents);
    yamlRemoveComments(lines, line_c);
    yamlRemoveEmptyLines(&lines, &line_c);

    for(index = 0; index < line_c; index++)
        printf("LINE:%s\n", lines[index]);

    for(index = 0; index < line_c; index++) 
        yamlParseLine(lines[index], index, &key_data, &key_c, &val_data, &val_c);

    for(index = 0; index < key_c; index++) 
        printf("KEY: %s, line %d, wsp %d\n", key_data[index].key_name, key_data[index].line, key_data[index].ws_c);
}