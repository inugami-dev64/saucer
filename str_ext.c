#include "saucer.h"

/* Remove whitespaces from end or beginning of the string 
 * and resize the buffer accordingly */
uint8_t trimStr(char **p_str, TrimMode trim_mode) {
    int32_t l_index;
    uint8_t is_content;
    // Trim front
    if((*p_str)) {
        if(trim_mode == TRIM_FRONT || trim_mode == TRIM_BOTH_SIDES) {
            is_content = false;
            
            // Skip whitespaces and quotation marks
            for(l_index = 0; l_index < strlen((*p_str)); l_index++) {
                if((*p_str)[l_index] != 0x20) {
                    is_content = true;
                    break;
                }
            }

            if(!strlen((*p_str)) || !is_content) return false;
            
            // Copy data to tmp_str
            char *tmp_str = (char*) calloc (
                strlen((*p_str)) - l_index + 1, 
                sizeof(char)
            );

            strncpy (
                tmp_str,
                (*p_str) + l_index,
                strlen((*p_str)) - l_index
            );

            // Copy data from tmp_str back to (*p_str)
            free((*p_str));
            
            (*p_str) = (char*) calloc (
                strlen(tmp_str) + 1,
                sizeof(char)
            );

            strncpy (
                (*p_str),
                tmp_str,
                strlen(tmp_str)
            );

            free(tmp_str);
        }
        
        // Trim the end of the string
        if(trim_mode == TRIM_END || trim_mode == TRIM_BOTH_SIDES) {
            is_content = false;

            // Skip all the whitespaces and quotation marks from the end
            for(l_index = strlen((*p_str)) - 1; l_index >= 0; l_index--) {
                if((*p_str)[l_index] != 0x20) {
                    is_content = true;
                    break;
                }
            }
            
            if(!strlen((*p_str)) || !is_content) return false;

            // Allocate memory for tmp_str and populate it
            char *tmp_str = (char*) calloc(
                l_index + 2, 
                sizeof(char)
            );

            strncpy (
                tmp_str,
                (*p_str),
                l_index + 1
            );

            free(*p_str);
            
            (*p_str) = (char*) calloc (
                strlen(tmp_str) + 1,
                sizeof(char)
            );

            strncpy (
                (*p_str),
                tmp_str,
                strlen(tmp_str)
            );

            free(tmp_str);
        }

        return true;
    }

    return false;
}


// Crop characters from string
void cropStr(char **p_str, TrimMode trim_mode, int c) {
    int32_t beg_index = 0; 
    int32_t end_index = (int32_t) strlen((*p_str));
    char *tmp_str;

    // Check for what to crop
    if(trim_mode == TRIM_FRONT || trim_mode == TRIM_BOTH_SIDES)
        beg_index = (int32_t) c;
    if(trim_mode == TRIM_END || trim_mode == TRIM_BOTH_SIDES)
        end_index = (int32_t) strlen((*p_str)) - ((int32_t) c);
    
    tmp_str = (char*) calloc (
        (end_index + 1) - beg_index,
        sizeof(char)
    );

    strncpy (
        tmp_str,
        (*p_str) + beg_index,
        end_index - beg_index
    );
    
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


/* Combine two string arrays into one array */
void cmbStrArr (
    char **arr1, 
    int32_t len1,
    char **arr2, 
    int32_t len2,
    char ***p_out_arr,
    int32_t *p_out_len
) {
    if(!len1 && !len2) return;
    int32_t l_index, r_index;
    (*p_out_len) = len1 + len2;
    (*p_out_arr) = (char**) calloc (
        (*p_out_len),
        sizeof(char*)
    );

    // Copy arr1 to out_arr
    for(l_index = 0, r_index = 0; l_index < len1; l_index++, r_index++)
        (*p_out_arr)[r_index] = arr1[l_index];

    // Copy arr2 to out_arr
    for(l_index = 0; l_index < len2; l_index++, r_index++)
        (*p_out_arr)[r_index] = arr2[l_index];
}


/* Make all chars in string to higher case */
void strToHigherCase(char **p_str) {
    int32_t l_index;
    for(l_index = 0; l_index < strlen((*p_str)); l_index++) {
        if
        (
            (*p_str)[l_index] >= MIN_LOWER_CASE &&
            (*p_str)[l_index] < MAX_LOWER_CASE 
        ) (*p_str)[l_index] -= CAPS_ADDABLE;
    }
}


/* Make all values of chars 0x00 */
void cleanStr(char *str, int32_t size) {
    int32_t index;
    for(index = 0; index < size; index++)
        str[index] = 0x00;
}