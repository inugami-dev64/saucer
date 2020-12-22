#ifndef YML_PARSER_H
#define YML_PARSER_H

#define ERRMEBEG "Saucer error: "
#define YAML_CH_BUFFER_SIZE 256

#define PERR(mes, li) printf("Parsing error: on line %d, %s\n", li, mes)
#define FERR(mes, file_name) printf("File error (%s): %s", mes, file_name)

#define true 1
#define false 0

#include <stdio.h>
#include <stdlib.h>

typedef struct StrBounds {
    int32_t str_beg;
    int32_t str_end;
} StrBounds;

typedef struct CharInfo {
    char ch;
    int32_t index;
} CharInfo;

typedef struct KeyData {
    int32_t colon_index;
    int32_t line;
    char *key_name;

    int32_t key_val_c;
    char *key_vals;
    int32_t ws_c;
} KeyData;

typedef struct ValueData {
    int32_t start_index;
    int32_t line;
    char *value_str;
    int32_t ws_c;
} ValueData;


typedef struct KeyPosInfo {
    int32_t min_index;
    int32_t max_index;
    uint8_t is_arr_elem;
} KeyPosInfo;


#ifdef YAML_PARSER_PRIVATE
    // Yaml parser helpers
    static uint8_t yamlSearchString(char *line, int32_t line_index, StrBounds **p_str_bds, int32_t *p_str_bds_c);
    static void yamlFindKeyNameBounds(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyPosInfo **p_kpi, int32_t *p_kpi_c);
    static void yamlExtractList(char *list, int32_t line_index, int32_t ws_c, int32_t list_index, ValueData **p_val_data, int32_t *p_val_c);
    static void yamlFindDirectValues(char *line, int32_t line_index, int32_t *kci, int32_t kci_c, ValueData **p_val_data, int32_t *p_val_c);
    static void yamlFindHyphenValues(char *line, int32_t line_index, int32_t hyph_index, ValueData **p_val_data, int32_t *p_val_c);


    // Main parsing functions 
    static void yamlLines(char ***p_lines, int32_t *p_size, char *file_contents);
    static void yamlRemoveComments(char **lines, int32_t lines_c);
    static void yamlRemoveEmptyLines(char ***p_lines, int32_t *p_size);
    static void yamlParseLine(char *line, int32_t line_index, KeyData **p_key_data, int32_t *p_key_c, ValueData **p_val_data, int32_t *p_val_c);
    static void yamlFindKeys(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyData **p_key_data, int32_t *p_key_c);
    static void yamlFindValues(char *line, int32_t line_index, ValueData **p_val_data, int32_t *p_val_c, KeyData *keys, int32_t key_c);
#endif

void yamlParse(const char *file_name);

#endif