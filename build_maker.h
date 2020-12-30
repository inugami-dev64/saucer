#ifndef BUILD_MAKER_H
#define BUILD_MAKER_H

/* Error message macro */
#define BERR(x, y) printf("Failed to create build config: %s%s\n", x, y)

/* Platform specifier */
typedef enum PlatformInfo {
    PLATFORM_ALL        = -1,
    PLATFORM_LINUX      = 0,
    PLATFORM_APPLE      = 1,
    PLATFORM_WINDOWS    = 2
} PlatformInfo;


/********** Premake structs **************/
/* Compiler to use */
typedef struct CompilerInfo {
    char *cc;
    char *cxx;
} CompilerInfo;


/* Generic compiler flags information */
struct GenFlagInfoBase {
    char **flags;
    int32_t flag_c;
};

typedef struct GenFlagInfoBase GenFlagInfoAll;
typedef struct GenFlagInfoBase GenFlagInfoWin;
typedef struct GenFlagInfoBase GenFlagInfoLinux;
typedef struct GenFlagInfoBase GenFlagInfoApple;

typedef struct GenFlagInfo {
    GenFlagInfoAll all;
    GenFlagInfoApple apple_i;
    GenFlagInfoLinux linux_i;
    GenFlagInfoWin win_i;
} GenFlagInfo;


/* Include flags information */
struct IncludePathInfoBase {
    char **paths;
    int32_t path_c;
};

typedef struct IncludePathInfoBase IncludePathInfoAll;
typedef struct IncludePathInfoBase IncludePathInfoWin;
typedef struct IncludePathInfoBase IncludePathInfoLinux;
typedef struct IncludePathInfoBase IncludePathInfoApple;

typedef struct IncludePathInfo {
    IncludePathInfoAll all;
    IncludePathInfoApple apple_i;
    IncludePathInfoLinux linux_i;
    IncludePathInfoWin win_i;
} IncludePathInfo;


/* Library paths information */
struct LibPathInfoBase {
    char **paths;
    int32_t path_c;
};

typedef struct LibPathInfoBase LibPathInfoAll;
typedef struct LibPathInfoBase LibPathInfoWin;
typedef struct LibPathInfoBase LibPathInfoLinux;
typedef struct LibPathInfoBase LibPathInfoApple;

typedef struct LibPathInfo {
    LibPathInfoAll all;
    LibPathInfoApple apple_i;
    LibPathInfoLinux linux_i;
    LibPathInfoWin win_i;
} LibPathInfo;


/* Project specifiers*/
typedef char* SrcDirPathInfo;
typedef char* BuildDirPathInfo;
typedef char* ProjectNameInfo;


/* Premake information */
typedef struct PremakeInfo {
    CompilerInfo cc_info;
    GenFlagInfo cc_flags;
    GenFlagInfo cxx_flags;
    IncludePathInfo incl_info;
    LibPathInfo lib_info;
    SrcDirPathInfo src_dir;
    BuildDirPathInfo build_dir;
    ProjectNameInfo project_name;
} PremakeInfo;


/************** Tasks structs ******************/
typedef char* TargetName;
typedef enum TargetType {
    TARGET_TYPE_DYNAMIC_LIBRARY = 0,
    TARGET_TYPE_STATIC_LIBRARY = 1,
    TARGET_TYPE_EXECUTABLE = 2
} TargetType;


/* Target source file specification */
struct TargetSrcInfoBase {
    char **files;
    int32_t file_c;
};

typedef struct TargetSrcInfoBase TargetSrcInfoAll;
typedef struct TargetSrcInfoBase TargetSrcInfoWin ;
typedef struct TargetSrcInfoBase TargetSrcInfoLinux;
typedef struct TargetSrcInfoBase TargetSrcInfoApple;

typedef struct TargetSrcInfo {
    TargetSrcInfoAll all;
    TargetSrcInfoApple apple_i;
    TargetSrcInfoLinux linux_i;
    TargetSrcInfoWin win_i;
} TargetSrcInfo;

typedef TargetSrcInfo TargetObjInfo;


/* Target dependencies specification */
/* 
 * Dependencies can only be applied to executables
 * Applying dependencies to library tasks results an makefile generation error 
 */
struct TargetDepsInfoBase {
    char **deps;
    int32_t deps_c;
};

typedef struct TargetDepsInfoBase TargetDepsInfoAll;
typedef struct TargetDepsInfoBase TargetDepsInfoWin;
typedef struct TargetDepsInfoBase TargetDepsInfoLinux;
typedef struct TargetDepsInfoBase TargetDepsInfoApple;

typedef struct TargetDepsInfo {
    TargetDepsInfoAll all;
    TargetDepsInfoApple apple_i;
    TargetDepsInfoLinux linux_i;
    TargetDepsInfoWin win_i;
} TargetDepsInfo;


/* Task specification struct */
typedef struct TaskInfo {
    TargetName name;
    TargetType type;
    TargetSrcInfo src_info;
    TargetObjInfo obj_info;
    TargetDepsInfo deps_info;
} TaskInfo;


/* Copy data */
typedef struct CopyInfoBase {
    char **dsts;
    char **srcs;
    int32_t cpy_c;
} CopyInfoBase;

typedef CopyInfoBase CopyInfoAll;
typedef CopyInfoBase CopyInfoWin;
typedef CopyInfoBase CopyInfoLinux;
typedef CopyInfoBase CopyInfoApple;

typedef struct CopyInfo {
    CopyInfoAll all;
    CopyInfoApple apple_i;
    CopyInfoLinux linux_i;
    CopyInfoWin win_i;
} CopyInfo;


/* Sym link data */
typedef struct LinkInfoBase {
    char **links;
    char **srcs;
    int32_t link_c;
} LinkInfoBase;

typedef LinkInfoBase LinkInfoAll;
typedef LinkInfoBase LinkInfoWin;
typedef LinkInfoBase LinkInfoLinux;
typedef LinkInfoBase LinkInfoApple;

typedef struct LinkInfo {
    LinkInfoAll all;
    LinkInfoApple apple_i;
    LinkInfoLinux linux_i;
    LinkInfoWin win_i;
} LinkInfo;


/* Build specification struct */
typedef struct BuildInfo {
    PlatformInfo platform;
    PremakeInfo premake;
    TaskInfo *tasks;
    int32_t task_c;
    CopyInfo cpy_info;
    LinkInfo link_info;
} BuildInfo;


#ifdef BUILD_MAKER_PRIVATE
    static void sauFindSrcFileExt(char *file);
    static void sauFindPlatformValues(KeyData *keys, int32_t key_c, int32_t cur_index, char ***p_vals, int32_t *p_val_c, char *platform);
    static void sauPushToStrArr(char ***p_dst_arr, int32_t *p_arr_len, char *val);

    // Premake config functions
    static void sauInitPremakeValues(BuildInfo *p_bi);
    static void sauVerifyPremake(BuildInfo *p_bi);
    static void sauFindPremakeValues(KeyData *keys, int32_t pre_beg_index, int32_t pre_end_index, BuildInfo *p_bi);
    static void sauAssemblePremake(KeyData *keys, int32_t key_c, BuildInfo *p_bi);

    // Tasks config functions
    static void sauGetTaskData(KeyData *keys, int32_t key_c, int32_t key_index, BuildInfo *p_bi);
    static void sauAssembleTasks(KeyData *keys, int32_t key_c, BuildInfo *p_bi);

    // Find links and copies
    static void sauFindLinks(KeyData *keys, int32_t key_c, BuildInfo *p_bi);
    static void sauFindCopies(KeyData *keys, int32_t key_c, BuildInfo *p_bi);
#endif

/* Create build info needed for makefile creation */
void sauAssembleBuildData(KeyData *keys, int32_t key_c, BuildInfo *p_bi);

#endif