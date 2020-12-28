#ifndef STR_EXT_H
#define STR_EXT_H

#define CAPS_ADDABLE 0x20
#define MIN_LOWER_CASE 0x61
#define MAX_LOWER_CASE 0x7B

typedef enum TrimMode {
    TRIM_FRONT = 0,
    TRIM_END = 1,
    TRIM_BOTH_SIDES = 2
} TrimMode;

uint8_t trimStr(char **p_str, TrimMode trim_mode);
void cmbStrArr(char **arr1, int32_t len1, char **arr2, int32_t len2, char ***p_out_arr, int32_t *p_out_len);
void cropStr(char **p_str, TrimMode trim_mode, int c);
void removeComments(char **line_contents, size_t n_lines);
void strToHigherCase(char **p_str);
void cleanStr(char *str, int32_t size);

#endif