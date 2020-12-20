#define YML_PARSER_PRIVATE 1
#include "saucer.h"

/* Find the most recent non sub key value (spaces count != -1)  */
static int32_t findLastNonSubKeyIndex(ListData *list_data, int32_t list_count) {
    int32_t index;
    for(index = list_count - 1; index >= 0; index--)
        if(list_data[index].spaces_count != -1) break;

    return index;
}


/* Delete all lines with no information on it */
static void cleanEmptyLines(char ***p_line_contents, size_t *p_line_count) {
    int32_t l_index, r_index;
    uint8_t found_content = false;
    int32_t tmp_lines_index = 0;
    char **tmp_lines = (char**) calloc(1, sizeof(char*));

    for(l_index = 0; l_index < (*p_line_count); l_index++) {
        // Check if string consists of only whitespaces
        for(r_index = 0; r_index < (int32_t) strlen((*p_line_contents)[l_index]); r_index++) {
            if((*p_line_contents)[l_index][r_index] != 0x20) {
                found_content = true;
                break;
            }
        }

        // Add string contents to temporary array
        if(found_content) {
            tmp_lines[tmp_lines_index] = (char*) calloc(strlen((*p_line_contents)[l_index]), sizeof(char));
            strcpy(tmp_lines[tmp_lines_index], (*p_line_contents)[l_index]);

            tmp_lines_index++;
            tmp_lines = (char**) realloc(tmp_lines, (tmp_lines_index + 1) * sizeof(char*));
            found_content = false;
        }
    }

    // Clean the current line contents
    for(l_index = 0; l_index < (*p_line_count); l_index++)
        free((*p_line_contents)[l_index]);
    
    free((*p_line_contents));
    (*p_line_count) = tmp_lines_index;

    // Copy tmp data to line contents
    (*p_line_contents) = (char**) calloc((*p_line_count), sizeof(char*));
    for(l_index = 0; l_index < (*p_line_count); l_index++) {
        (*p_line_contents)[l_index] = (char*) calloc(strlen(tmp_lines[l_index]), sizeof(char));
        strcpy((*p_line_contents)[l_index], tmp_lines[l_index]);
    }

    // Clean tmp line contents
    for(l_index = 0; l_index < tmp_lines_index + 1; l_index++)
        free(tmp_lines[l_index]);
    
    free(tmp_lines);
}


/* Find string in line */
static uint8_t findString(char *line, int32_t line_index, int32_t *p_min, int32_t *p_max) {
    int32_t index;
    char mark;
    uint8_t is_str_found = false;
    uint8_t is_str_closed = false;

    // Find first quotation mark
    for(index = 0; index < (int32_t) strlen(line); index++) {
        // 0x27 == ''' && 0x22 == '"'
        if(line[index] == 0x27 || line[index] == 0x22) {
            mark = line[index];
            (*p_min) = index;
            is_str_found = true;
            index++;
            break;
        }
    }

    // Find second quotation mark
    for(; index < (int32_t) strlen(line); index++) {
        if(line[index] == mark) {
            (*p_max) = index;
            is_str_closed = true;
            break;
        }
    }

    // Check for parsing error
    if(!is_str_closed && is_str_found) {
        PERR("unclosed string", line_index);
        exit(-1);
    }

    return is_str_found;
}


/* Find the nearest key value */ 
static uint8_t findKeyName(char *line, int32_t line_index, char **p_key_name, int32_t *p_beg_sp, int32_t start_index, int32_t end_index) {
    int32_t index, ch_index; 
    uint8_t colon_found = false;
    int32_t min_str = 0, max_str = 0;
    char *tmp_str = calloc(YML_LIST_NAME_BUF_SIZE, sizeof(char));
    uint8_t is_string = findString(line, line_index, &min_str, &max_str);

    if(end_index == -1) end_index = (int32_t) strlen(line);

    for(index = start_index, ch_index = 0; index < end_index - 1; index++, ch_index++) {
        if(!is_string || (index < min_str || index > max_str)) {
            if(line[index] == 0x3A) {
                colon_found = true;
                break;
            }

            if(line[index] == 0x7B) return false;

        }
        tmp_str[ch_index] = line[index];
    }

    if(!colon_found) return false;
    // Find the amount of whitespaces
    if(p_beg_sp)
        for((*p_beg_sp) = 0; (*p_beg_sp) < strlen(tmp_str) && tmp_str[(*p_beg_sp)] == 0x20; (*p_beg_sp)++);

    trimStr(&tmp_str, TRIM_BOTH_SIDES);

    (*p_key_name) = (char*) calloc(strlen(tmp_str), sizeof(char));
    strcpy((*p_key_name), tmp_str);
    free(tmp_str);
    return true;
}


/* Extract all array elements into separate lists */ 
static uint8_t handleMultiArray(char *line, int32_t line_index, ListData **p_lists, size_t *p_list_count) {
    int32_t l_index, r_index, ch_index;
    int32_t beg_index, end_index;
    int32_t elem_count = 0;
    int32_t *elem_br_pt = (int32_t*) calloc(elem_count + 1, sizeof(int32_t));

    // Find array starting parenthesis
    // 0x7B == '{'
    for(beg_index = 0; beg_index < strlen(line) && line[beg_index] != 0x7B; beg_index++);
    if(beg_index >= strlen(line)) return false;

    // Check if array is closed
    uint8_t is_arr_closed = false;
    for(end_index = 0; end_index < strlen(line); end_index++) {
        if(line[end_index] == 0x7D) {
            is_arr_closed = true;
            break;
        }
    }
    
    // Check for parsing error
    if(!is_arr_closed) {
        PERR("unclosed array", line_index);
        exit(-1);
    }

    // Find all array element breakpoints
    int32_t prev_pos = beg_index;
    for(l_index = prev_pos; l_index < end_index; l_index++) {
        if(line[l_index] == 0x2C) {
            elem_count++;
            elem_br_pt = (int32_t*) realloc(elem_br_pt, sizeof(int32_t) * elem_count);

            elem_br_pt[elem_count - 1] = l_index;
            prev_pos = elem_br_pt[elem_count];
        }
    }

    // Find all keys and their values from the array
    char *tmp_name, *tmp_str;
    prev_pos = beg_index;
    uint8_t is_key_found;
    for(l_index = 0; l_index < elem_count + 1; l_index++) {
        // Search for key name
        if(l_index == elem_count) {
            is_key_found = findKeyName(line, line_index, &tmp_name, NULL, prev_pos + 1, end_index - 1);

            // Find colon
            for(r_index = prev_pos + 1; r_index < end_index; r_index++)
                if(line[r_index] == 0x3A) break;    
        }

        else {
            is_key_found = findKeyName(line, line_index, &tmp_name, NULL, prev_pos + 1, elem_br_pt[l_index]);
            // Find colon
            for(r_index = prev_pos + 1; r_index < elem_br_pt[l_index]; r_index++)
                if(line[r_index] == 0x3A) break;    
        }
        if(!is_key_found) {
            PERR("no key specified in double multidimentional array", line_index + 1);
            exit(-1);
        }
        
        // Check for parsing error
        if(line[r_index + 1] != 0x20) {
            PERR("expected whitespace after key declaration", line_index);
            exit(-1);
        }

        // Skip whitespaces
        for(r_index++; r_index < elem_br_pt[l_index] && line[r_index] == 0x20; r_index++);
        
        tmp_str = (char*) calloc(elem_br_pt[l_index], sizeof(char));

        if(l_index != elem_count)
            // Populate the tmp_str variable
            for(ch_index = 0; r_index < elem_br_pt[l_index]; r_index++, ch_index++)
                tmp_str[ch_index] = line[r_index];
        else
            for(ch_index = 0; r_index < end_index; r_index++, ch_index++)
                tmp_str[ch_index] = line[r_index];


        (*p_list_count)++;
        (*p_lists) = (ListData*) realloc((*p_lists), sizeof(ListData) * (*p_list_count));
        (*p_lists)[(*p_list_count) - 1].spaces_count = -1;
        (*p_lists)[(*p_list_count) - 1].line_index = line_index;
        (*p_lists)[(*p_list_count) - 1].list_memb_size = 1;
        
        (*p_lists)[(*p_list_count) - 1].list_membs = (char**) calloc(1, sizeof(char*));
        (*p_lists)[(*p_list_count) - 1].list_membs[0] = (char*) calloc(strlen(tmp_str), sizeof(char));
        strcpy((*p_lists)[(*p_list_count) - 1].list_membs[0], tmp_str);
        
        (*p_lists)[(*p_list_count) - 1].list_name = (char*) calloc(strlen(tmp_name), sizeof(char));
        strcpy((*p_lists)[(*p_list_count) - 1].list_name, tmp_name);
        
        free(tmp_str);
        free(tmp_name);
        
        prev_pos = elem_br_pt[l_index];
    }

    return true;
}

/* Check if line belongs to list declaration */
static uint8_t isKeyLine(char *line, int32_t line_index, int32_t *p_ws, char **p_name) {
    int32_t l_index, r_index;
    uint8_t is_list = false;
    int32_t tmp_ws;
    int32_t min_str, max_str;
    char tmp_str[YML_LIST_NAME_BUF_SIZE] = {0};
    uint8_t is_str = findString(line, line_index, &min_str, &max_str);

    // Count the amount of spaces
    for(tmp_ws = 0; tmp_ws < strlen(line) && line[tmp_ws] == 0x20; tmp_ws++);

    // Find the name of the list
    for(l_index = strlen(line) - 1; l_index >= 0; l_index--) {
        // Add chars to tmp_str in reverse order 
        if(is_list) {
            if(line[l_index] == 0x20) break;
            tmp_str[r_index] = line[l_index];
            r_index++;
        }
        else if(!is_str || (l_index < min_str || l_index > max_str)) {
            if(line[l_index] == 0x3A)
                is_list = true;
        }
    }

    // Rearrange chars into p_name
    if(is_list && p_name != NULL) {
        (*p_name) = (char*) calloc(strlen(tmp_str), sizeof(char));
        for(l_index = strlen(tmp_str) - 1, r_index = 0; l_index >= 0; l_index--, r_index++)
            (*p_name)[r_index] = tmp_str[l_index];
    }

    if(is_list) (*p_ws) = tmp_ws;

    return is_list;
}


/* Find list members per line */
static void scanForLineValues(char **line_contents, size_t n_lines, int32_t line_beg, ListData **p_list_data, size_t *p_list_size) {
    int32_t min_ws = (*p_list_data)[(*p_list_size) - 1].spaces_count;
    int32_t max_ws = YML_LIST_NAME_BUF_SIZE;
    int32_t n_ws, tmp_ws = YML_LIST_NAME_BUF_SIZE;
    int32_t l_index, r_index, ch_index;
    
    int32_t list_index;
    for(l_index = line_beg; l_index < n_lines && min_ws < max_ws; l_index++) {
        n_ws = 0;
        // Check if list is defined on line
        if(!isKeyLine(line_contents[l_index], l_index, &tmp_ws, NULL)) {
            // Skip all whitespaces
            for(r_index = 0; r_index < strlen(line_contents[l_index]); r_index++, n_ws++)
                if(line_contents[l_index][r_index] != 0x20) break;

            if(n_ws <= max_ws && n_ws > min_ws) {
                // Check for parsing error
                // 0x2D == '-'
                if(line_contents[l_index][r_index] == 0x2D && line_contents[l_index][r_index + 1] != 0x20) {
                    PERR("expected whitespace after '-'", l_index + 1);
                    exit(-1);
                }

                else if(line_contents[l_index][r_index] == 0x2D) {
                    // Skip all whitespaces
                    while(r_index < strlen(line_contents[l_index]) && line_contents[l_index][r_index] == 0x20) r_index++;
                    // Check for parsing error
                    if(r_index >= strlen(line_contents[l_index])) {
                        PERR("expected value after '-'", l_index);
                        exit(-1);
                    }

                    r_index++;
                    list_index = findLastNonSubKeyIndex((*p_list_data), *p_list_size);
                    (*p_list_data)[list_index].list_memb_size++;
                    (*p_list_data)[list_index].list_membs = (char**) realloc((*p_list_data)[list_index].list_membs, (*p_list_data)[list_index].list_memb_size * sizeof(char*));
                    (*p_list_data)[list_index].list_membs[(*p_list_data)[list_index].list_memb_size - 1] = (char*) calloc(1, sizeof(char));

                    (*p_list_data)[list_index].list_membs[(*p_list_data)[list_index].list_memb_size - 1] = (char*) 
                    calloc(strlen(line_contents[l_index]) - 1, sizeof(char));

                    // Find the member
                    for(ch_index = 0; r_index < strlen(line_contents[l_index]); r_index++, ch_index++)
                        (*p_list_data)[list_index].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1][ch_index] = line_contents[l_index][r_index];

                    trimStr(&(*p_list_data)[list_index].list_membs[(*p_list_data)[list_index].list_memb_size - 1], TRIM_BOTH_SIDES);
                }
            }
        }

        else {
            handleMultiArray(line_contents[l_index], l_index, p_list_data, p_list_size);
            return;
        }

        if(tmp_ws < min_ws) break;
        if(tmp_ws < max_ws) max_ws = tmp_ws;
    }
}


/* Add all non key members to listdata */
static void findKeyMembers(char **line_contents, size_t line_size, int32_t line_index, ListData **p_list_data, size_t *p_list_size) {
    int32_t l_index, r_index;
    
    // Find the colon
    for(l_index = 0; l_index < strlen(line_contents[line_index]); l_index++) {
        if(line_contents[line_index][l_index] == 0x3A) break; 
        if(line_contents[line_index][l_index] == 0x7B) {
            printf("detected double array parenthesis, leaving %d\n", line_index + 1);
            return;
        }
    }

    // Check for parsing error
    l_index++;
    if(l_index < strlen(line_contents[line_index]) && line_contents[line_index][l_index] != 0x20) {
        PERR("expected whitespace after ':'", line_index + 1);
        exit(-1);
    }

    // Skip all whitespaces
    while(l_index < strlen(line_contents[line_index]) && line_contents[line_index][l_index] == 0x20) l_index++;
    if(l_index >= strlen(line_contents[line_index])) return;

    // Detected list of elements
    // 0x5B == '['
    if(line_contents[line_index][l_index] == 0x5B) {
        uint8_t is_array_closed = false;
        (*p_list_data)[(*p_list_size) - 1].list_memb_size++;
        (*p_list_data)[(*p_list_size) - 1].list_membs = (char**) realloc((*p_list_data)[(*p_list_size) - 1].list_membs, sizeof(char*) * (*p_list_data)[(*p_list_size) - 1].list_memb_size);
        (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1] = (char*) calloc(1, sizeof(char));
        r_index = 0;

        // Find array contents
        l_index++;
        for(; l_index < strlen(line_contents[line_index]); l_index++) {
            // 0x2C == ',' and 0x5D == ']'
            if(line_contents[line_index][l_index] != 0x2C && line_contents[line_index][l_index] != 0x5D) {
                (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1][r_index] = line_contents[line_index][l_index];
                r_index++;
                (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1] = (char*) 
                realloc((*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1], (r_index + 1) * sizeof(char));
            }

            // Found comma in array, so reallocate memory for next string
            else if(line_contents[line_index][l_index] == 0x2C) {
                r_index = 0;
                (*p_list_data)[(*p_list_size) - 1].list_memb_size++;
                (*p_list_data)[(*p_list_size) - 1].list_membs = (char**) realloc((*p_list_data)[(*p_list_size) - 1].list_membs, ((*p_list_data)[(*p_list_size) - 1].list_memb_size + 1) * sizeof(char*));
                (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1] = (char*) calloc(1, sizeof(char));
                
                // Skip commas and whitespaces
                while(line_contents[line_index][l_index] == 0x2C || line_contents[line_index][l_index] == 0x20) l_index++;
                l_index--;
            }

            // Found the closing parenthesis
            else if(line_contents[line_index][l_index] == 0x5D) {
                is_array_closed = true;
                break;
            }
        }
        
        // Check for the parsing error
        if(!is_array_closed) {
            PERR("expected closing parenthesis on line", line_index + 1);
            exit(-1);
        }
    }

    // No array members, but found a member on list line 
    else if(l_index != strlen(line_contents[line_index]) - 1 && 
    line_contents[line_index][l_index] != 0x7B) {
        (*p_list_data)[(*p_list_size) - 1].list_memb_size++;
        (*p_list_data)[(*p_list_size) - 1].list_membs = (char**) realloc((*p_list_data)[(*p_list_size) - 1].list_membs, (*p_list_data)[(*p_list_size) - 1].list_memb_size * sizeof(char*));
        r_index = 0;
        (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1] = (char*) calloc(1, sizeof(char));
        
        // Write chars to member string
        for(r_index = 0; l_index < strlen(line_contents[line_index]); l_index++, r_index++) {
            (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1] = (char*) 
            realloc((*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1], sizeof(char) * (r_index + 1));
            (*p_list_data)[(*p_list_size) - 1].list_membs[(*p_list_data)[(*p_list_size) - 1].list_memb_size - 1][r_index] = line_contents[line_index][l_index];
        }
    }

    // Trim strings
    for(l_index = 0; l_index < (*p_list_data)[(*p_list_size) - 1].list_memb_size; l_index++)
        trimStr(&(*p_list_data)[(*p_list_size) - 1].list_membs[l_index], TRIM_BOTH_SIDES);
}


// Find all non list data inside a list
static void populateListData(char **line_contents, size_t line_size, ListData **p_list_data, size_t *p_list_size) {
    int32_t index;
    uint8_t list_found = false;
    char *tmp_name;
    int32_t n_ws = 0;
    size_t cpy_list_size;

    (*p_list_size) = 0;
    (*p_list_data) = (ListData*) calloc(1, sizeof(ListData));

    // Find the list data
    for(index = 0; index < line_size; index++) {
        n_ws = 0;
        // Check if list is found
        if(line_contents[index]);
        list_found = findKeyName(line_contents[index], index, &tmp_name, &n_ws, 0, strlen(line_contents[index]));

        if(list_found) {           
            // Realloc memory for new ListData struct instance
            (*p_list_size)++;
            cpy_list_size = (*p_list_size);
            (*p_list_data) = (ListData*) realloc((*p_list_data), cpy_list_size * sizeof(ListData));
            (*p_list_data)[cpy_list_size - 1].line_index = index;
            (*p_list_data)[cpy_list_size - 1].list_memb_size = 0;
            (*p_list_data)[cpy_list_size - 1].spaces_count = n_ws;
            (*p_list_data)[cpy_list_size - 1].list_name = (char*) calloc(strlen(tmp_name), sizeof(char));
            strcpy((*p_list_data)[(*p_list_size) - 1].list_name, tmp_name);
            free(tmp_name);

            // Find all list members
            findKeyMembers(line_contents, line_size, index, p_list_data, &cpy_list_size);
            handleMultiArray(line_contents[index], index, p_list_data, p_list_size);
            scanForLineValues(line_contents, line_size, index + 1, p_list_data, p_list_size);

            list_found = false;
        }
    }
}


/* Parse the yaml file */
void parseYaml(const char *file_name) {
    size_t l_index, r_index;
    // Read all file contents into char buffer
    size_t res;
    FILE *file;
    file = fopen(file_name, "rb");
    if(!file) printf("%s%s%s\n", ERRMEBEG, "Failed to open file ", file_name);

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t) ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read all file contents
    char *contents = (char*) malloc(sizeof(char) * file_size);
    res = fread(contents, sizeof(char), file_size, file);
    if(!res) printf("%s%s%s\n", ERRMEBEG, "Failed to read file ", file_name);

    // Extract all file information into lines
    char **line_data = (char**) calloc(1, sizeof(char*));
    size_t line_count = 1;
    
    line_data[0] = (char*) calloc(1, sizeof(char));
    r_index = 0;

    for(l_index = 0; l_index < strlen(contents); l_index++) {
        // Allocate more memory for more lines
        if(contents[l_index] == 0x0A) {
            line_count++;
            line_data = (char**) realloc((void*) line_data, line_count * sizeof(char*));
            line_data[line_count - 1] = (char*) calloc(1, sizeof(char));
            line_data[line_count - 1][0] = 0x20;
            r_index = 0;
        }
        // Populate memory with line chars
        else {
            line_data[line_count - 1][r_index] = contents[l_index];
            r_index++;
            line_data[line_count - 1] = (char*) realloc(line_data[line_count - 1], sizeof(char) * (r_index + 1));
        }
    }

    ListData *list_data;
    size_t list_size;

    removeComments(line_data, line_count);
    cleanEmptyLines(&line_data, &line_count);
    populateListData(line_data, line_count, &list_data, &list_size);

    for(l_index = 0; l_index < list_size; l_index++) {
        printf("%s, %d:\n", list_data[l_index].list_name, list_data[l_index].spaces_count);
        for(r_index = 0; r_index < list_data[l_index].list_memb_size; r_index++)
            printf("--%s\n", list_data[l_index].list_membs[r_index]);
    }
        

    parseCleanup(list_data, list_size);
} 


/* Perform cleanup after parsing */
void parseCleanup(ListData *list_data, size_t b_size) {
    size_t index;
    for(index = 0; index < b_size; index++)
        free(list_data[index].list_membs);

    free(list_data);
}