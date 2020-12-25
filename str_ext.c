#include "saucer.h"

/* Remove whitespaces from end or beginning of the string 
 * and resize the buffer accordingly */
uint8_t trimStr(char **p_str, TrimMode trim_mode) {
    int32_t l_index, r_index;
    uint8_t is_content;
    // Trim front
    if((*p_str)) {
        if(trim_mode == TRIM_FRONT || trim_mode == TRIM_BOTH_SIDES) {
            is_content = false;
            char *tmp_str = (char*) calloc (
                strlen((*p_str)), 
                sizeof(char)
            );

            strncpy (
                tmp_str, 
                (*p_str), 
                strlen((*p_str))
            );
            
            // Skip whitespaces and quotation marks
            for(l_index = 0; l_index < strlen(tmp_str); l_index++) {
                if(tmp_str[l_index] != 0x20) {
                    is_content = true;
                    break;
                }
            }

            if(!strlen(tmp_str) || !is_content) return false;

            // Free and allocate space according to the new string size
            free((*p_str));
            (*p_str) = (char*) calloc (
                strlen(tmp_str) - l_index + 1, 
                sizeof(char)
            );

            for(r_index = 0; l_index < strlen(tmp_str); l_index++, r_index++)
                (*p_str)[r_index] = tmp_str[l_index];
            
            (*p_str)[r_index] = 0x00;
            free(tmp_str);
        }
        
        // Trim the end of the string
        if(trim_mode == TRIM_END || trim_mode == TRIM_BOTH_SIDES) {
            is_content = false;
            char *tmp_str = (char*) calloc (
                strlen((*p_str)) + 1, 
                sizeof(char)
            );
            
            strncpy (
                tmp_str, 
                (*p_str), 
                strlen((*p_str))
            );
            
            tmp_str[strlen((*p_str))] = 0x00;

            // Skip all the whitespaces and quotation marks from the end
            for(l_index = strlen(tmp_str) - 1; l_index >= 0; l_index--) {
                if(tmp_str[l_index] != 0x20) {
                    is_content = true;
                    break;
                }
            }
            
            if(!strlen(tmp_str) || !is_content) return false;

            // Free and allocate space according to the new string size
            free((*p_str));
            (*p_str) = (char*) calloc (
                l_index + 1, 
                sizeof(char)
            );

            l_index++;
            
            strncpy (
                (*p_str), 
                tmp_str, 
                l_index
            );
            
            (*p_str)[l_index] = 0x00;
            free(tmp_str);
        }

        return true;
    }

    return false;
}


// Crop characters from string
void cropStr(char **p_str, TrimMode trim_mode, int c) {
    int32_t l_index, r_index;
    int32_t beg_index = 0; 
    int32_t end_index = (int32_t) strlen((*p_str));
    char *tmp_str;

    // Check for what to crop
    if(trim_mode == TRIM_FRONT || trim_mode == TRIM_BOTH_SIDES)
        beg_index = (int32_t) c;
    if(trim_mode == TRIM_END || trim_mode == TRIM_BOTH_SIDES)
        end_index = (int32_t) strlen((*p_str)) - ((int32_t) c);
    
    tmp_str = (char*) calloc(YAML_CH_BUFFER_SIZE, sizeof(char));

    // Copy chars to tmp_str
    for(l_index = beg_index, r_index = 0; l_index < end_index - beg_index + 1; l_index++, r_index++)
        tmp_str[r_index] = (*p_str)[l_index];
    
    // Change *p_str pointer to tmp_str pointer
    free((*p_str));
    (*p_str) = (char*) calloc(strlen(tmp_str) + 1, sizeof(char));
    strncpy((*p_str), tmp_str, strlen(tmp_str));

    free(tmp_str);
}


/* Remove all comments in lines */
void removeComments(char **line_contents, size_t n_lines) {
    int32_t l_index, r_index;
    uint8_t is_comment_found = false;
    char *tmp_str;

    // Find all comments and remove them
    for(l_index = 0; l_index < (int32_t) n_lines; l_index++) {
        for(r_index = 0; r_index < strlen(line_contents[l_index]); r_index++) {
            // 0x23 == '#'
            if(line_contents[l_index][r_index] == 0x23) { 
                is_comment_found = true; 
                break;
            }
        }

        // Copy the uncommented string to line_contents
        if(is_comment_found && r_index) {
            tmp_str = (char*) calloc(r_index + 1, sizeof(char));
            strncpy(tmp_str, line_contents[l_index], (size_t) (r_index));
            
            free(line_contents[l_index]);
            line_contents[l_index] = (char*) calloc((size_t) r_index + 1, sizeof(char));
            
            strncpy(line_contents[l_index], tmp_str, strlen(tmp_str));
            free(tmp_str);
        }

        else if(!r_index) {
            free(line_contents[l_index]);
            line_contents[l_index] = (char*) calloc(1, sizeof(char));
            line_contents[l_index][0] = 0x20;
        }
    }
}


/* Make all values of chars 0x00 */
void cleanStr(char *str, int32_t size) {
    int32_t index;
    for(index = 0; index < size; index++)
        str[index] = 0x00;
}