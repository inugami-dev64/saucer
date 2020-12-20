#ifndef YML_PARSER_H
#define YML_PARSER_H

#define ERRMEBEG "Saucer error: "
#define YML_LIST_NAME_BUF_SIZE 256

#define PERR(mes, li) printf("Parsing error: on line %d, %s\n", li, mes)
#define FERR(mes, file_name) printf("File error (%s): %s", mes, file_name)

#define true 1
#define false 0

#include <stdio.h>
#include <stdlib.h>

// Specifing the platform for compiling
typedef enum Platform {
    LINUX = 0,
    WINDOWS = 1,
    MACOS = 2
} Platform;


// Specify the library type
typedef enum LibraryType {
    STATIC = 0,
    DYNAMIC = 1
} LibraryType;


// Specify output path
typedef struct OutputPath {
    char *outdir;
    char *outlibdir;
} OutputData;


// Line parsing flags
typedef enum LineMemberFlags {
    LINE_MEMB_UNDEFINED = 0,
    LINE_ALL_COMMENT = 1,
    LINE_BLOCK = 2
} LineMemberFlags;


typedef struct ListData {
    char *list_name;
    size_t *sub_block_indices;
    size_t sub_block_ind_size;

    int32_t spaces_count;
    int32_t line_index;
    
    char **list_membs;
    size_t list_memb_size;
} ListData; 


// Store information about build
typedef struct Preinit {
    Platform platform;
    char *cc;
    char *cc_flags;
    char *cxx;
    char *cxx_flags;
    OutputData outdata;

    char **incl_dirs;
    size_t n_incl_dirs;

    char **lib_dirs;
    size_t n_lib_dirs;
} Preinit;


// Executable task
typedef struct ExecTask {
    char *target;

    char **src_files;
    size_t n_src_files;

    char **link_libs;
    size_t n_link_libs;
} ExecTask;


// Library build task
typedef struct LibTask {
    char *target;
    LibraryType lib_type;

    char **src_files;
    size_t n_src_files;

    char **link_libs;
    size_t n_link_libs;
} LibTask;


#ifdef YML_PARSER_PRIVATE
    #define LINE_MEMB_INDEX_SUCCESS 0 
    #define LINE_MEMB_INDEX_ERROR_NOT_FOUND -1
    #define LINE_MEMB_INDEX_ERROR_BLOCK -2
    #define LINE_MEMB_INDEX_ERROR_COMMENT -3

    /* Helpers (unparsed)*/
    /* Return false if no string is detected on a line */
    static uint8_t findString(char *line, int32_t line_index, int32_t *p_min, int32_t *p_max);
    /* Return false if line contains key declaration */
    static uint8_t isKeyLine(char *line, int32_t line_index, int32_t *p_ws, char **p_name);
    /* Get the name of the key on current line */
    static uint8_t findKeyName(char *line, int32_t line_index, char **p_key_name, int32_t *p_beg_sp, int32_t start_index, int32_t end_index);
    static void cleanEmptyLines(char ***p_line_contents, size_t *p_line_count);
    static int32_t findLastNonSubKeyIndex(ListData *list_data, int32_t list_count);

    /* Main private parsing functions */
    static uint8_t handleMultiArray(char *line, int32_t line_index, ListData **p_lists, size_t *p_data_count);
    static void scanForLineValues(char **line_contents, size_t n_lines, int32_t line_beg, ListData **p_list_data, size_t *p_list_size);
    static void findKeyMembers(char **line_contents, size_t line_size, int32_t line_index, ListData **p_list_data, size_t *p_list_size);
    static void populateListData(char **line_contents, size_t line_size, ListData **block_data, size_t *data_count);
#endif

void parseYaml(const char *file_name);
void parseCleanup(ListData *block_data, size_t b_size);

#endif