#ifndef STR_EXT_H
#define STR_EXT_H

typedef enum TrimMode {
    TRIM_FRONT = 0,
    TRIM_END = 1,
    TRIM_BOTH_SIDES = 2
} TrimMode;

void trimStr(char **p_str, TrimMode trim_mode);
void removeComments(char **line_contents, size_t n_lines);

#endif