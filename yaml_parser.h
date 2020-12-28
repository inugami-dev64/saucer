#ifndef YML_PARSER_H
#define YML_PARSER_H

#define ERRMEBEG "Saucer error: "
#define YAML_CH_BUFFER_SIZE 256

#define PERR(mes, li) printf("Parsing error: on line %d, %s\n", li, mes)
#define FERR(mes, file_name) printf("File error (%s): %s", mes, file_name)

#define true 1
#define false 0

typedef struct StrBounds {
    int32_t str_beg;
    int32_t str_end;
} StrBounds;

typedef struct CharInfo {
    char ch;
    int32_t index;
} CharInfo;

typedef struct KeyData {
    uint8_t is_sub_key;
    int32_t colon_index;
    int32_t line;
    char *key_name;

    int32_t ws_c;
    char **key_vals;
    int32_t key_val_c;
} KeyData;


typedef struct KeyPosInfo {
    int32_t min_index;
    int32_t max_index;
    uint8_t is_arr_elem;
} KeyPosInfo;


#ifdef YAML_PARSER_PRIVATE
    // Yaml parser helpers
    static uint8_t yamlSearchString(char *line, int32_t line_index, StrBounds **p_str_bds, int32_t *p_str_bds_c);
    static uint8_t yamlFindKeyNameBounds(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyPosInfo **p_kpi, int32_t *p_kpi_c);
    static void yamlExtractList(char *list, KeyData *p_key);
    static void yamlGetKeyLineValues(char *line, KeyData *p_key, char *p_op, uint8_t *p_is_op);
    static uint8_t yamlGetGenLineValues(char *line, KeyData *p_key, int32_t min_ws, int32_t max_ws, char op, uint8_t is_op);
    static void yamlPushKeyValue(char *val, KeyData *p_key);


    // Main parsing functions 
    static void yamlLines(char ***p_lines, int32_t *p_size, char *file_contents);
    static void yamlRemoveComments(char **lines, int32_t lines_c);
    static void yamlRemoveEmptyLines(char ***p_lines, int32_t *p_size);
    static void yamlParseKeys(char **lines, int32_t line_c, int32_t *p_li_index, KeyData **p_key_data, int32_t *p_key_c);
    static void yamlParseValues(char **lines, int32_t line_c, KeyData *keys, int32_t key_c, int32_t key_index);
    static void yamlFindKeys(char *line, int32_t line_index, int32_t min_index, int32_t max_index, KeyData **p_key_data, int32_t *p_key_c);
#endif

void yamlParse(KeyData **p_keys, int32_t *p_key_c, char *file_name);

#endif