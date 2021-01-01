#define MAKE_WRITER_PRIVATE
#include "saucer.h"

static uint8_t is_library_targets = false;
static uint8_t is_cpy = false;
static uint8_t is_link = false;

static char **lib_targets;
static int32_t lib_tar_c;

static FILE *file;

/* Write simple single line variable */
static void sauWriteSingleLineVar (
    char *var, 
    char *var_val
) {
    char *tmp_str = (char*) calloc (
        strlen(var) + strlen(var_val) + 5,
        sizeof(char)
    );

    // Write variable declaration into tmp_str
    sprintf (
        tmp_str, 
        "%s = %s\n", 
        var, 
        var_val
    );
    
    // Write variable declaration into makefile
    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);
}


/* Write flag information into variable */
static void sauWriteFlagsVar (
    char *var, 
    char **flags, 
    int32_t flag_c 
) {
    int32_t l_index, b_offset;
    char *tmp_str;

    // Count the total number of chars in Makefile variable declaration string
    int32_t len = strlen(var) + 4;
    for(l_index = 0; l_index < flag_c; l_index++)
        len += (strlen(flags[l_index]) + 2);

    // Allocate memory for tmp_str
    tmp_str = (char*) calloc (
        len + 2,
        sizeof(char)
    );

    // Print var_spec data to tmp_str 
    sprintf (
        tmp_str,
        "%s =",
        var
    );

    // Copy all values to tmp_str
    b_offset = strlen(var) + 2;
    for(l_index = 0; l_index < flag_c; l_index++) {
        sprintf (
            tmp_str + b_offset,
            " -%s",
            flags[l_index]
        );

        b_offset += (2 + strlen(flags[l_index]));
    }

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    fwrite("\n", sizeof(char), 1, file);
    free(tmp_str);
}


/* Write multiline variables */
static void sauWriteMultiLineVar (
    char *var,
    char *flag, 
    char **var_vals, 
    int32_t val_c 
) {
    int32_t l_index, r_index;
    int32_t ws_c = strlen(var) + 2;
    char *tmp_str;
    char *ws;
    
    // Write the first line
    if(val_c == 1) {
        tmp_str = (char*) calloc (
            strlen(var) + strlen(flag) + strlen(*var_vals) + 6,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "%s =%s %s\n",
            var,
            flag,
            *var_vals
        );

        fwrite (
            tmp_str, 
            sizeof(char), 
            strlen(tmp_str), 
            file
        );

        free(tmp_str);
    }

    else {
        tmp_str = (char*) calloc (
            strlen(var) + strlen(flag) + strlen(*var_vals) + 8,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "%s =%s %s \\\n",
            var, 
            flag,
            *var_vals
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    // Write the following value lines
    
    for(l_index = 1; l_index < val_c; l_index++) {
        tmp_str = (char*) calloc (
            ws_c + strlen(flag) + strlen(var_vals[l_index]) + 6,
            sizeof(char)
        );

        // Allocate memory for whitespace string
        ws = (char*) calloc (
            ws_c + 1,
            sizeof(char)
        );

        // Populate whitespace string
        for(r_index = 0; r_index < ws_c; r_index++)
            ws[r_index] = 0x20;

        // Check if current value is last one
        if(l_index == val_c - 1) {
            sprintf (
                tmp_str,
                "%s%s %s\n",
                ws,
                flag,
                var_vals[l_index]
            );
        }

        else {
            sprintf (
                tmp_str,
                "%s%s %s \\\n",
                ws,
                flag,
                var_vals[l_index]
            );
        }

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(ws);
        free(tmp_str);
    }
}


/* Convert normal path into windows style path ('/' ==> '\') */
void sauFixWindowsPaths(char **paths, int32_t path_c) {
    int32_t l_index, r_index;

    for(l_index = 0; l_index < path_c; l_index++) {
        for(r_index = 0; r_index < strlen(paths[l_index]); r_index++) {
            if
            (
                paths[l_index][r_index] == 0x2F &&
                r_index != strlen(paths[l_index]) - 1
            ) paths[l_index][r_index] = 0x5C;

            else if
            (
                r_index == strlen(paths[l_index]) - 1 &&
                (
                    paths[l_index][r_index] == 0x2F ||
                    paths[l_index][r_index] == 0x5C
                )
            ) paths[l_index][r_index] = 0x00;
        }
    }
}


/* Count the amount of C++ and C language sources */
static void sauCountSrcTypes (
    char **srcs,
    int32_t src_c,
    int32_t *p_c_src_c,
    int32_t *p_cxx_src_c
) {
    int32_t l_index, r_index;
    for(l_index = 0; l_index < src_c; l_index++) {
        // Find the extension path
        for(r_index = strlen(srcs[l_index]) - 1; r_index >= 0; r_index--)
            if(srcs[l_index][r_index] == 0x2E) break;

        r_index++;
        
        if
        (
            !strncmp(srcs[l_index] + r_index, "cpp", 3) ||
            !strncmp(srcs[l_index] + r_index, "cxx", 3) ||
            !strncmp(srcs[l_index] + r_index, "cc", 2) 
        ) (*p_cxx_src_c)++;
        else (*p_c_src_c)++;
    }
}


/* Push default library path to library path */
static void sauPushDefaultLibraryPath (
    BuildInfo *p_bi, 
    char ***p_arr, 
    int32_t *p_arr_len
) {
    // Reallocate more memory for library path array
    (*p_arr_len)++;
    int32_t cur_index = (*p_arr_len) - 1;
    char **tmp_paths = (char**) realloc (
        (*p_arr),
        (*p_arr_len) * sizeof(char*)
    );

    // Check for memory allocation error
    if(!tmp_paths) {
        MEMERR("failed to allocate memory for library paths");
        exit(ERRC_MEM);
    }

    (*p_arr) = tmp_paths;

    // Find the total size of the default path string
    int32_t ch_c = 0;
    if(p_bi->premake.project_name[strlen(p_bi->premake.build_dir) - 1] == 0x2F) 
        ch_c += strlen(p_bi->premake.build_dir);
    else ch_c += (strlen(p_bi->premake.build_dir) + 1);
    
    if(p_bi->premake.build_dir[strlen(p_bi->premake.project_name) - 1] == 0x2F)
        ch_c += strlen(p_bi->premake.project_name);
    else ch_c += (strlen(p_bi->premake.project_name) + 1);
    ch_c += 4;

    // Allocate memory for default path string
    (*p_arr)[cur_index] = (char*) calloc (
        ch_c,
        sizeof(char)
    );

    // Construct the default path string
    int32_t offset = 0;
    if(p_bi->premake.build_dir[strlen(p_bi->premake.build_dir) - 1] == 0x2F) {
        sprintf (
            (*p_arr)[cur_index] + offset,
            "%s",
            p_bi->premake.build_dir
        );
        offset += strlen(p_bi->premake.build_dir);
    }
    else {
        sprintf (
            (*p_arr)[cur_index] + offset,
            "%s/",
            p_bi->premake.build_dir
        );
        offset += (strlen(p_bi->premake.build_dir) + 1);
    }

    if(p_bi->premake.project_name[strlen(p_bi->premake.project_name) - 1] == 0x2F) {
        sprintf (
            (*p_arr)[cur_index] + offset,
            "%s",
            p_bi->premake.project_name
        );
        offset += strlen(p_bi->premake.project_name);
    }
    else {
        sprintf (
            (*p_arr)[cur_index] + offset,
            "%s/",
            p_bi->premake.project_name
        );
        offset += (strlen(p_bi->premake.project_name) + 1);
    }

    sprintf (
        (*p_arr)[cur_index] + offset,
        "%s",
        "lib"
    );
}


/* Write generic platform specific variable to Makefile */
static uint8_t sauWritePlatformFlagVar (
    char **all_str, 
    int32_t all_str_c, 
    char **pl_str, 
    int32_t pl_str_c, 
    char *var_name
) {
    char **tmp_str_arr = NULL;
    int32_t tmp_len = 0;

    cmbStrArr (
        all_str,
        all_str_c,
        pl_str,
        pl_str_c,
        &tmp_str_arr,
        &tmp_len
    );

    if(tmp_len) {
        sauWriteFlagsVar (
            var_name,
            tmp_str_arr,
            tmp_len
        );
        
        free(tmp_str_arr);
    }

    return (tmp_len != 0);
}


/* Write platform specifice multiline variable */
static uint8_t sauWritePlatformMultilineVar
(
    char **all_str,
    int32_t all_str_c,
    char **pl_str,
    int32_t pl_str_c,
    char *var_name,
    char *fl
) {
    char **tmp_str_arr;
    int32_t tmp_len = 0;

    cmbStrArr (
        all_str, 
        all_str_c,
        pl_str,
        pl_str_c,
        &tmp_str_arr,
        &tmp_len
    );

    if(tmp_len) {
        sauWriteMultiLineVar (
            var_name,
            fl,
            tmp_str_arr,
            tmp_len
        );

        free(tmp_str_arr);
    }

    return (tmp_len != 0);   
}


static void sauGetPlatformSpecificObjNames (
    char **srcs,
    int32_t src_c,
    char ***p_objs,
    int32_t *p_obj_c,
    char *target_name
) {
    int32_t l_index, r_index;
    int32_t str_offset;

    // Allocate memory for *p_objs
    (*p_obj_c) = src_c;
    (*p_objs) = (char**) calloc (
        (*p_obj_c),
        sizeof(char*)
    );

    for(l_index = 0; l_index < src_c; l_index++) {
        str_offset = 0;
        // Find the correct offset
        for(r_index = strlen(srcs[l_index]) - 1; r_index >= 0; r_index--) {
            if(srcs[l_index][r_index] == 0x2F) {
                !r_index ? (str_offset = 0) : (str_offset = r_index + 1);
                break;
            }
        }

        (*p_objs)[l_index] = (char*) calloc (
            strlen(srcs[l_index] + str_offset) + strlen(target_name) + 
            strlen(BUILD_DIR_VAR) + 12,
            sizeof(char)
        );

        sprintf (
            (*p_objs)[l_index],
            "$(%s)/obj/%s/%s.o",
            BUILD_DIR_VAR,
            target_name,
            srcs[l_index] + str_offset
        );
    }
}


/* Get library linking flags according to platform */
static char* sauGetPlatformSpecificDependencies (
    TaskInfo *p_task,
    PlatformInfo pi
) {
    char *tmp_str;
    char *test_str;
    int32_t l_index;

    int32_t ch_c = 1;
    int32_t str_offset = 0;

    // Find non platform specific link flags char count
    for(l_index = 0; l_index < p_task->deps_info.all.deps_c; l_index++)
        ch_c += (strlen(p_task->deps_info.all.deps[l_index]) + 2);

    // Allocate memory for non platform specific link flags
    if(ch_c) {
        tmp_str = (char*) calloc (
            ch_c,
            sizeof(char)
        );
    }

    else {
        tmp_str = (char*) calloc (
            1,
            sizeof(char)
        );
    }

    // Populate the string with non platform specific link flags
    for(l_index = 0; l_index < p_task->deps_info.all.deps_c; l_index++) {
        sprintf (
            tmp_str + str_offset,
            " -%s",
            p_task->deps_info.all.deps[l_index]
        );

        str_offset += strlen(p_task->deps_info.all.deps[l_index]) + 2;
    }

    switch (pi)
    {
    case PLATFORM_APPLE:
        // Find to total count of apple link flag chars
        for(l_index = 0; l_index < p_task->deps_info.apple_i.deps_c; l_index++)
            ch_c += (strlen(p_task->deps_info.apple_i.deps[l_index]) + 2);

        // Reallocate more memory for tmp_str
        test_str = (char*) realloc (
            tmp_str,
            ch_c * sizeof(char)
        );

        if(!test_str) {
            MEMERR("failed to reallocate memory for library flags");
            exit(ERRC_MEM);
        }

        tmp_str = test_str;

        // Populate the string with link flags
        for(l_index = 0; l_index < p_task->deps_info.apple_i.deps_c; l_index++) {
            sprintf (
                tmp_str + str_offset,
                " -%s",
                p_task->deps_info.apple_i.deps[l_index]
            );

            str_offset += strlen(p_task->deps_info.apple_i.deps[l_index]) + 2;
        }
        break;
    
    case PLATFORM_LINUX:
        // Find to total count of linux link flag chars
        for(l_index = 0; l_index < p_task->deps_info.linux_i.deps_c; l_index++)
            ch_c += (strlen(p_task->deps_info.linux_i.deps[l_index]) + 2);

        // Reallocate more memory for tmp_str
        test_str = (char*) realloc (
            tmp_str,
            ch_c * sizeof(char)
        );

        if(!test_str) {
            MEMERR("failed to reallocate memory for library flags");
            exit(ERRC_MEM);
        }

        tmp_str = test_str;

        // Populate the string with link flags
        for(l_index = 0; l_index < p_task->deps_info.linux_i.deps_c; l_index++) {
            sprintf (
                tmp_str + str_offset,
                " -%s",
                p_task->deps_info.linux_i.deps[l_index]
            );

            str_offset += strlen(p_task->deps_info.linux_i.deps[l_index]) + 2;
        }
        break;

    case PLATFORM_WINDOWS:
        // Find to total count of windows link flag chars
        for(l_index = 0; l_index < p_task->deps_info.win_i.deps_c; l_index++)
            ch_c += (strlen(p_task->deps_info.win_i.deps[l_index]) + 2);

        // Reallocate more memory for tmp_str
        test_str = (char*) realloc (
            tmp_str,
            ch_c * sizeof(char)
        );

        if(!test_str) {
            MEMERR("failed to reallocate memory for library flags");
            exit(ERRC_MEM);
        }

        tmp_str = test_str;

        // Populate the string with link flags
        for(l_index = 0; l_index < p_task->deps_info.win_i.deps_c; l_index++) {
            sprintf (
                tmp_str + str_offset,
                " -%s",
                p_task->deps_info.win_i.deps[l_index]
            );

            str_offset += strlen(p_task->deps_info.win_i.deps[l_index]) + 2;
        }
        break;
    default:
        break;
    }

    return tmp_str;
}


/* Write library path variable */
static uint8_t sauWriteLibraryPathVar
(
    char **all_str,
    int32_t all_str_c,
    char **pl_str,
    int32_t pl_str_c,
    char *lib_var,
    uint8_t is_imported,
    BuildInfo *p_bi
) {
    char **tmp_str_arr = NULL;
    int32_t tmp_len = 0;

    cmbStrArr (
        all_str,
        all_str_c,
        pl_str,
        pl_str_c,
        &tmp_str_arr,
        &tmp_len
    );

    // Reallocate memory for base library path if needed
    if(is_library_targets && !is_imported)
        sauPushDefaultLibraryPath(p_bi, &tmp_str_arr, &tmp_len);
    
    if(tmp_len) {
        if(p_bi->platform == PLATFORM_WINDOWS) 
            sauFixWindowsPaths(tmp_str_arr, tmp_len);

        sauWriteMultiLineVar (
            lib_var,
            LIBRARY_FLAG,
            tmp_str_arr,
            tmp_len
        );

        free(tmp_str_arr);
    }

    return (tmp_len != 0);
}


/* Create variable from project name */
static char *sauMakeVarName(char *var_base, char *pr_name, uint8_t is_imported) {
    char *out_str;
    // Check if variable is part of imported config
    if(is_imported) {
        out_str = (char*) calloc (
            strlen(var_base) + strlen(pr_name) + 2,
            sizeof(char)
        );

        sprintf (
            out_str,
            "%s_%s",
            pr_name,
            var_base
        );

        strToHigherCase(&out_str);
    }

    else {
        out_str = (char*) calloc (
            strlen(var_base) + 1,
            sizeof(char)
        );

        sprintf (
            out_str,
            "%s",
            var_base
        );
    }

    return out_str;
}


/* Write all premake variables */
static void sauWritePremake(BuildInfo *p_bi, uint8_t is_imported) {
    // Check if src and build paths need to be made compatible with windows
    if(p_bi->platform == PLATFORM_WINDOWS) {
        sauFixWindowsPaths(&p_bi->premake.src_dir, 1);
        sauFixWindowsPaths(&p_bi->premake.build_dir, 1);
    }

    // Find source variable name
    char *src_var = sauMakeVarName (
        SRC_DIR_VAR, 
        p_bi->premake.project_name, 
        is_imported
    );

    // Find C flags variable name
    char *c_flags_var = sauMakeVarName (
        C_FLAGS_VAR,
        p_bi->premake.project_name,
        is_imported
    );

    // Find C++ flags variable name
    char *cxx_flags_var = sauMakeVarName (
        CPP_FLAGS_VAR,
        p_bi->premake.project_name,
        is_imported
    );

    // Find include path variable name
    char *incl_path_var = sauMakeVarName (
        INCLUDE_PATH_VAR,
        p_bi->premake.project_name,
        is_imported
    );

    // Find library path variable name
    char *lib_path_var = sauMakeVarName (
        LIBRARY_PATH_VAR,
        p_bi->premake.project_name,
        is_imported
    );
    
    // Write src directory var
    sauWriteSingleLineVar (
        src_var,
        p_bi->premake.src_dir
    );


    if(!is_imported) {
        // Write build directory var
        sauWriteSingleLineVar (
            BUILD_DIR_VAR,
           p_bi->premake.build_dir
        );
    }

    // Allocate memory for 
    switch (p_bi->platform)
    {
    // Write with macos config
    case PLATFORM_APPLE:
        // Copy CC_FLAGS data
        p_bi->flag_infos.is_cc_flags = sauWritePlatformFlagVar (
            p_bi->premake.cc_flags.all.flags,
            p_bi->premake.cc_flags.all.flag_c,
            p_bi->premake.cc_flags.apple_i.flags,
            p_bi->premake.cc_flags.apple_i.flag_c,
            c_flags_var
        );

        // Copy CXX_FLAGS data
        p_bi->flag_infos.is_cxx_flags = sauWritePlatformFlagVar (
            p_bi->premake.cxx_flags.all.flags,
            p_bi->premake.cxx_flags.all.flag_c,
            p_bi->premake.cxx_flags.apple_i.flags,
            p_bi->premake.cxx_flags.apple_i.flag_c,
            cxx_flags_var
        );

        // Copy INCLUDE_PATH data
        p_bi->flag_infos.is_incl_path = sauWritePlatformMultilineVar (
            p_bi->premake.incl_info.all.paths,
            p_bi->premake.incl_info.all.path_c,
            p_bi->premake.incl_info.apple_i.paths,
            p_bi->premake.incl_info.apple_i.path_c,
            incl_path_var,
            INCLUDE_FLAG
        );

        // Copy LIBRARY_PATH data
        p_bi->flag_infos.is_lib_path = sauWriteLibraryPathVar (
            p_bi->premake.lib_info.all.paths,
            p_bi->premake.lib_info.all.path_c,
            p_bi->premake.lib_info.apple_i.paths,
            p_bi->premake.lib_info.apple_i.path_c,
            lib_path_var,
            is_imported,
            p_bi
        );

        break;

    // Write with linux config
    case PLATFORM_LINUX:
        // Copy CC_FLAGS data
        p_bi->flag_infos.is_cc_flags = sauWritePlatformFlagVar (
            p_bi->premake.cc_flags.all.flags,
            p_bi->premake.cc_flags.all.flag_c,
            p_bi->premake.cc_flags.linux_i.flags,
            p_bi->premake.cc_flags.linux_i.flag_c,
            c_flags_var
        );

        // Copy CXX_FLAGS data
        p_bi->flag_infos.is_cxx_flags = sauWritePlatformFlagVar (
            p_bi->premake.cxx_flags.all.flags,
            p_bi->premake.cxx_flags.all.flag_c,
            p_bi->premake.cxx_flags.linux_i.flags,
            p_bi->premake.cxx_flags.linux_i.flag_c,
            cxx_flags_var
        );

        // Copy INCLUDE_PATH data
        p_bi->flag_infos.is_incl_path = sauWritePlatformMultilineVar (
            p_bi->premake.incl_info.all.paths,
            p_bi->premake.incl_info.all.path_c,
            p_bi->premake.incl_info.linux_i.paths,
            p_bi->premake.incl_info.linux_i.path_c,
            incl_path_var,
            INCLUDE_FLAG
        );

        // Copy LIBRARY_PATH data
        p_bi->flag_infos.is_lib_path = sauWriteLibraryPathVar (
            p_bi->premake.lib_info.all.paths,
            p_bi->premake.lib_info.all.path_c,
            p_bi->premake.lib_info.linux_i.paths,
            p_bi->premake.lib_info.linux_i.path_c,
            lib_path_var,
            is_imported,
            p_bi
        );
        break;

    // Write with windows config
    case PLATFORM_WINDOWS:
        // Make paths windows compatible
        sauFixWindowsPaths (
            p_bi->premake.incl_info.all.paths,
            p_bi->premake.incl_info.all.path_c
        );
        sauFixWindowsPaths (
            p_bi->premake.incl_info.win_i.paths,
            p_bi->premake.incl_info.win_i.path_c
        );

        // Copy CC_FLAGS data
        p_bi->flag_infos.is_cc_flags = sauWritePlatformFlagVar (
            p_bi->premake.cc_flags.all.flags,
            p_bi->premake.cc_flags.all.flag_c,
            p_bi->premake.cc_flags.win_i.flags,
            p_bi->premake.cc_flags.win_i.flag_c,
            c_flags_var
        );

        // Copy CXX_FLAGS data
        p_bi->flag_infos.is_cxx_flags = sauWritePlatformFlagVar (
            p_bi->premake.cxx_flags.all.flags,
            p_bi->premake.cxx_flags.all.flag_c,
            p_bi->premake.cxx_flags.win_i.flags,
            p_bi->premake.cxx_flags.win_i.flag_c,
            cxx_flags_var
        );

        // Copy INCLUDE_PATH data
        p_bi->flag_infos.is_incl_path = sauWritePlatformMultilineVar (
            p_bi->premake.incl_info.all.paths,
            p_bi->premake.incl_info.all.path_c,
            p_bi->premake.incl_info.win_i.paths,
            p_bi->premake.incl_info.win_i.path_c,
            incl_path_var,
            INCLUDE_FLAG
        );

        // Copy LIBRARY_PATH data
        p_bi->flag_infos.is_lib_path = sauWriteLibraryPathVar (
            p_bi->premake.lib_info.all.paths,
            p_bi->premake.lib_info.all.path_c,
            p_bi->premake.lib_info.win_i.paths,
            p_bi->premake.lib_info.win_i.path_c,
            lib_path_var,
            is_imported,
            p_bi
        );
        break;
    
    default:
        break;
    }

    free(c_flags_var);
    free(cxx_flags_var);
    free(src_var);
    free(incl_path_var);
    free(lib_path_var);
}


/* Find if library targets exist */
static void sauFindLibTargets(BuildInfo *p_bi) {
    int32_t l_index;

    for(l_index = 0; l_index < p_bi->task_c; l_index++) {
        if
        (
            p_bi->tasks[l_index].type == TARGET_TYPE_DYNAMIC_LIBRARY ||
            p_bi->tasks[l_index].type == TARGET_TYPE_STATIC_LIBRARY
        ) {
            is_library_targets = true;
            return;
        }
    }
}


/* Write all task premake variables */
static void sauWriteTaskTargetVar (
    PlatformInfo pi, 
    TaskInfo *p_task, 
    char *project_name
) {
    char *tmp_str;
    // Create target name variable
    char *target_var = (char*) calloc (
        strlen(p_task->name) + 8,
        sizeof(char)
    );

    sprintf (
        target_var,
        "%s_TARGET",
        p_task->name
    );

    strToHigherCase(&target_var);

    // Create tmp_str of target variable and its value
    switch (p_task->type)
    {
    case TARGET_TYPE_DYNAMIC_LIBRARY:
        if(pi == PLATFORM_WINDOWS) {
            tmp_str = (char*) calloc (
                strlen(target_var) + strlen(p_task->name) + 
                strlen(project_name) + strlen(BUILD_DIR_VAR) + 18,
                sizeof(char)
            );

            sprintf (
                tmp_str,
                "%s = $(%s)/%s/lib/%s.dll\n",
                target_var,
                BUILD_DIR_VAR,
                project_name,
                p_task->name
            );
        }

        else {
            tmp_str = (char*) calloc (
                strlen(target_var) + strlen(p_task->name) + 
                strlen(project_name) + strlen(BUILD_DIR_VAR) + 17,
                sizeof(char)
            );

            sprintf (
                tmp_str,
                "%s = $(%s)/%s/lib/%s.so\n",
                target_var,
                BUILD_DIR_VAR,
                project_name,
                p_task->name
            );
        }

        break;
    
    case TARGET_TYPE_STATIC_LIBRARY:
        tmp_str = (char*) calloc (
            strlen(target_var) + strlen(p_task->name) + 
            strlen(project_name) + strlen(BUILD_DIR_VAR) + 16,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "%s = $(%s)/%s/lib/%s.a\n",
            target_var,
            BUILD_DIR_VAR,
            project_name,
            p_task->name
        );
        break;

    case TARGET_TYPE_EXECUTABLE:
        tmp_str = (char*) calloc (
            strlen(target_var) + strlen(p_task->name) + 
            strlen(project_name) + strlen(BUILD_DIR_VAR) + 10,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "%s = $(%s)/%s/%s\n",
            target_var,
            BUILD_DIR_VAR,
            project_name,
            p_task->name
        );
        break;
    
    default:
        break;
    }

    if(pi == PLATFORM_WINDOWS)
        sauFixWindowsPaths(&tmp_str, 1);

    fwrite (
        tmp_str, 
        sizeof(char), 
        strlen(tmp_str), 
        file
    );

    free(tmp_str);

    if(p_task->type == TARGET_TYPE_EXECUTABLE) free(target_var);
    else {
        lib_tar_c++;
        char **tmp_arr = (char**) realloc( 
            lib_targets,
            sizeof(char*) * lib_tar_c
        );

        if(!tmp_arr) {
            MEMERR("failed to reallocate memory for library targets");
            exit(ERRC_MEM);
        }

        lib_targets = tmp_arr;
        lib_targets[lib_tar_c - 1] = target_var;
    }
}


/* Write target object variable */
static void sauWriteTaskObjVar(PlatformInfo pi, TaskInfo *p_task) {

    // Get the object variable name
    char *obj_var = (char*) calloc (
        strlen(p_task->name) + 5,
        sizeof(char)
    );

    sprintf (
        obj_var,
        "%s_OBJ",
        p_task->name
    );

    strToHigherCase(&obj_var);

    switch (pi)
    {
    case PLATFORM_APPLE:
        sauWritePlatformMultilineVar (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.apple_i.files,
            p_task->obj_info.apple_i.file_c,
            obj_var,
            ""
        );
        break;

    case PLATFORM_LINUX:
        sauWritePlatformMultilineVar (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.linux_i.files,
            p_task->obj_info.linux_i.file_c,
            obj_var,
            ""
        );
        break;

    case PLATFORM_WINDOWS:
        sauFixWindowsPaths (
            p_task->src_info.all.files,
            p_task->src_info.all.file_c
        );

        sauFixWindowsPaths (
            p_task->src_info.win_i.files,
            p_task->src_info.win_i.file_c
        );

        sauFixWindowsPaths (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c
        );

        sauFixWindowsPaths (
            p_task->obj_info.win_i.files,
            p_task->obj_info.win_i.file_c
        );

        sauWritePlatformMultilineVar (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.win_i.files,
            p_task->obj_info.win_i.file_c,
            obj_var,
            ""
        );
        break;
    
    default:
        break;
    }
}


/* Write main task into makefile */
static void sauWriteMainTask (
    TaskInfo *p_task,
    CompilerFlagInfos *p_fi,
    int32_t c_src_c,
    int32_t cxx_src_c,
    char *target,
    char *objs,
    PlatformInfo pi
) {
    int32_t l_index;
    char *tmp_str;

    int32_t line_ch_c = 0;
    int32_t offset = 0;
    // Copy first main target specification line to tmp_str
    if(p_task->type == TARGET_TYPE_EXECUTABLE) {
        // Find the line size
        line_ch_c += (strlen(target) + 4);
        line_ch_c += (strlen(SCRIPT_NAME) + 2);
        line_ch_c += strlen(objs) + 4;
        for(l_index = 0; l_index < lib_tar_c; l_index++) 
            line_ch_c += strlen(lib_targets[l_index]) + 4;
        line_ch_c += 2;

        // Allocate memory for tmp_str
        tmp_str = (char*) calloc (
            line_ch_c,
            sizeof(char)
        );

        // Populate tmp_str
        sprintf (
            tmp_str + offset,
            "$(%s): .%s $(%s)",
            target,
            SCRIPT_NAME,
            objs
        );

        offset += (strlen(target) + strlen(SCRIPT_NAME) + strlen(objs) + 10);
        for(l_index = 0; l_index < lib_tar_c; l_index++) {
            sprintf (
                tmp_str + offset,
                " $(%s)",
                lib_targets[l_index]
            );

            offset += (strlen(lib_targets[l_index]) + 4);
        }

        sprintf (
            tmp_str + offset,
            "\n"
        );
    }

    else {
        line_ch_c = (strlen(target) + strlen(objs) + strlen(SCRIPT_NAME) + 12);
        tmp_str = (char*) calloc (
            line_ch_c,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "$(%s): .%s $(%s)\n",
            target,
            SCRIPT_NAME,
            objs
        );
    }

    // Write first line of main task
    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);

    int32_t ch_c = 0;
    switch (p_task->type)
    {
    case TARGET_TYPE_DYNAMIC_LIBRARY:
        if(!cxx_src_c) {
            ch_c = 27 + strlen(target) + strlen(objs);
            if(p_fi->is_cc_flags) ch_c += strlen(C_FLAGS_VAR) + 4;

            tmp_str = (char*) calloc (
                ch_c,
                sizeof(char)
            );

            offset = 0;
            sprintf (
                tmp_str + offset,
                "\t$(%s) -shared -o $(%s) $(%s)",
                C_COMPILER_VAR,
                target,
                objs
            );

            offset += strlen(tmp_str);
            if(p_fi->is_cc_flags) {
                sprintf(tmp_str + offset, " $(%s)", C_FLAGS_VAR);
                offset += strlen(C_FLAGS_VAR) + 4;
            }
            sprintf(tmp_str + offset, "\n");
        }

        else {
            ch_c = 28 + strlen(target) + strlen(objs);
            if(p_fi->is_cxx_flags) ch_c += strlen(CPP_FLAGS_VAR) + 4;
            
            tmp_str = (char*) calloc (
                ch_c,
                sizeof(char)
            );

            offset = 0;
            sprintf (
                tmp_str + offset,
                "\t$(%s) -shared -o $(%s) $(%s)",
                CPP_COMPILER_VAR,
                target,
                objs
            );

            offset += strlen(tmp_str);
            if(p_fi->is_cxx_flags) {
                sprintf(tmp_str + offset, " $(%s)", CPP_FLAGS_VAR);
                offset += strlen(CPP_FLAGS_VAR) + 4;
            }

            sprintf(tmp_str + offset, "\n");
        }
        break;

    case TARGET_TYPE_STATIC_LIBRARY:
        tmp_str = (char*) calloc (
            strlen(target) + strlen(objs) + 16,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\tar rs $(%s) $(%s)\n",
            target,
            objs
        );
        break;

    case TARGET_TYPE_EXECUTABLE: {
        ch_c = strlen(target) + strlen(objs) + 19;
        
        if(!cxx_src_c) ch_c += strlen(C_COMPILER_VAR) + 3;
        else ch_c += strlen(CPP_COMPILER_VAR) + 3;
        if
        (
            p_fi->is_cc_flags &&
            !cxx_src_c
        ) ch_c += strlen(C_FLAGS_VAR) + 4;
        else if
        (
            p_fi->is_cxx_flags &&
            cxx_src_c
        ) ch_c += strlen(CPP_FLAGS_VAR) + 4;
        if(p_fi->is_incl_path) ch_c += strlen(INCLUDE_PATH_VAR) + 4;
        if(p_fi->is_lib_path) ch_c += strlen(LIBRARY_PATH_VAR) + 4;
        char *lib_fl = sauGetPlatformSpecificDependencies(p_task, pi);
        if(*lib_fl) ch_c += strlen(lib_fl);

        tmp_str = (char*) calloc (
            ch_c,
            sizeof(char)
        );  

        offset = 0;
        if(!cxx_src_c) {
            sprintf (
                tmp_str + offset,
                "\t$(%s) -o $(%s) $(%s)",
                C_COMPILER_VAR,
                target,
                objs
            );
        }

        else {
            sprintf (
                tmp_str + offset,
                "\t$(%s) -o $(%s) $(%s)",
                CPP_COMPILER_VAR,
                target,
                objs
            );
        }

        offset += strlen(tmp_str);

        if 
        (
            p_fi->is_cc_flags &&
            c_src_c >= cxx_src_c
        ) {
            sprintf (
                tmp_str + offset, 
                " $(%s)",
                C_FLAGS_VAR
            );
            offset += strlen(C_FLAGS_VAR) + 4;
        }

        else if 
        (
            p_fi->is_cxx_flags &&
            c_src_c < cxx_src_c
        ) {
            sprintf (
                tmp_str + offset,
                " $(%s)",
                CPP_FLAGS_VAR
            );
            offset += strlen(CPP_FLAGS_VAR) + 4;
        }

        if(p_fi->is_incl_path) {
            sprintf (
                tmp_str + offset,
                " $(%s)",
                INCLUDE_PATH_VAR
            );
            offset += strlen(INCLUDE_PATH_VAR) + 4;
        }

        if(p_fi->is_lib_path) {
            sprintf (
                tmp_str + offset,
                " $(%s)",
                LIBRARY_PATH_VAR
            );
            offset += strlen(LIBRARY_PATH_VAR) + 4;
        }

        if(*lib_fl) {
            sprintf (
                tmp_str + offset,
                "%s",
                lib_fl
            );
            offset += strlen(lib_fl);
            free(lib_fl);
        }

        sprintf(tmp_str + offset, "\n");
        break;
    }
    
    default:
        break;
    }

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);
}


/* Write task subtask into makefile */
static void sauWriteSubtask (
    char *obj, 
    char *src,
    TargetType tar_type,
    CompilerFlagInfos *p_fi,
    char *src_var,
    PlatformInfo pi 
) {
    int32_t is_c = 0;
    int32_t is_cpp = 0;
    char *tmp_str;
    // Find the source file type
    sauCountSrcTypes(&src, 1, &is_c, &is_cpp);

    // Write the task name and dependencies
    tmp_str = (char*) calloc (
        strlen(obj) + strlen(src) +
        strlen(src_var) + 8,
        sizeof(char)
    );

    if(pi == PLATFORM_WINDOWS)
        sprintf (
            tmp_str,
            "%s: $(%s)\\%s\n",
            obj,
            src_var,
            src
        );
    else 
        sprintf (
            tmp_str,
            "%s: $(%s)/%s\n",
            obj,
            src_var,
            src
        );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);

    // Write the task specification 
    int32_t ch_c = strlen(src_var) + strlen(src) + strlen(obj);
    if(is_c) 
        ch_c += strlen(C_COMPILER_VAR) + 25;
    else if(is_cpp)
        ch_c += strlen(CPP_COMPILER_VAR) + 25;
    if
    (
        is_c &&
        p_fi->is_cc_flags
    ) ch_c += strlen(C_FLAGS_VAR) + 4;
    else if
    ( 
        is_cpp &&
        p_fi->is_cxx_flags
    ) ch_c += strlen(CPP_FLAGS_VAR) + 4;
    if(p_fi->is_incl_path) ch_c += strlen(INCLUDE_PATH_VAR) + 4;
    if(tar_type == TARGET_TYPE_DYNAMIC_LIBRARY) ch_c += 6;

    // Allocate memory for tmp_str
    tmp_str = (char*) calloc (
        ch_c,
        sizeof(char)
    );

    int32_t offset = 0;
    switch (tar_type)
    {
    case TARGET_TYPE_DYNAMIC_LIBRARY:
        if(is_c) {
            if(pi == PLATFORM_WINDOWS)
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)\\%s -o %s -fPIC",
                    C_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );

            else 
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)/%s -o %s -fPIC",
                    C_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );
        }
        else {
            if(pi == PLATFORM_WINDOWS)
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)\\%s -o %s -fPIC",
                    CPP_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );

            else
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)/%s -o %s -fPIC",
                    CPP_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );
        }
        break;

    case TARGET_TYPE_STATIC_LIBRARY:
    case TARGET_TYPE_EXECUTABLE:
        if(is_c) {
            if(pi == PLATFORM_WINDOWS)
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)\\%s -o %s",
                    C_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );

            else 
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)/%s -o %s",
                    C_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );
        }
        else {
            if(pi == PLATFORM_WINDOWS)
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)\\%s -o %s",
                    CPP_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );

            else
                sprintf (
                    tmp_str + offset,
                    "\t$(%s) -c $(%s)/%s -o %s",
                    CPP_COMPILER_VAR,
                    src_var,
                    src,
                    obj
                );
        }
        break;
    
    default:
        break;
    }

    offset += strlen(tmp_str);

    if 
    (
        is_c &&
        p_fi->is_cc_flags
    ) { 
        sprintf (
            tmp_str + offset,
            " $(%s)",
            C_FLAGS_VAR
        );
        offset += strlen(C_FLAGS_VAR) + 4;
    }

    else if 
    (
        is_cpp &&
        p_fi->is_cxx_flags
    ) {
        sprintf (
            tmp_str + offset,
            " $(%s)",
            CPP_FLAGS_VAR
        );
        offset += strlen(CPP_FLAGS_VAR) + 4;
    }
    
    if(p_fi->is_incl_path) {
        sprintf (
            tmp_str + offset,
            " $(%s)",
            INCLUDE_PATH_VAR
        );
        offset += strlen(INCLUDE_PATH_VAR) + 4;    
    }

    sprintf (
        tmp_str + offset,
        "\n"
    );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);
}


/* Write initial task into makefile (executed via generated script) */
static void sauWriteInitTask(PlatformInfo pi) {
    char *tmp_str;
    // Declare task in makefile
    tmp_str = (char*) calloc (
        strlen(SCRIPT_NAME) + 4,
        sizeof(char)
    );

    sprintf (
        tmp_str,
        ".%s:\n",
        SCRIPT_NAME
    );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);

    // Create task specification string according to the build platform
    if(pi != PLATFORM_WINDOWS) {
        tmp_str = (char*) calloc (
            (2 * strlen(SCRIPT_NAME)) + strlen(BUILD_DIR_VAR) + 26,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\tchmod +x %s.sh\n" \
            "\t./%s.sh $(%s)\n",
            SCRIPT_NAME,
            SCRIPT_NAME,
            BUILD_DIR_VAR
        );
    }

    else {
        tmp_str = (char*) calloc (
            strlen(SCRIPT_NAME) + strlen(BUILD_DIR_VAR) + 13,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\t.\\%s.bat $(%s)\n",
            SCRIPT_NAME,
            BUILD_DIR_VAR
        );
    }

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);
}


/* Write default make task (performs all the tasks) */
static void sauWriteAllTask (
    TaskInfo *tasks,
    int32_t task_c,
    BuildInfo *imports,
    int32_t im_c
) {
    int32_t l_index, r_index;
    char *tmp_str;

    // Write the all task declaration
    fwrite (
        ALL_TASK_NAME \
        ":",
        sizeof(char),
        4,
        file
    );

    // Check for copy and link targets
    if(is_cpy) {
        fwrite (
            " " \
            CPY_TASK,
            sizeof(char),
            1 + strlen(CPY_TASK),
            file
        );
    }

    if(is_link) {
        fwrite (
            " " \
            LINK_TASK,
            sizeof(char),
            1 + strlen(LINK_TASK),
            file
        );
    }

    // Add all main targets 
    for(l_index = 0; l_index < task_c; l_index++) {
        tmp_str = (char*) calloc (
            strlen(tasks[l_index].name) + 12,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            " $(%s_TARGET)",
            tasks[l_index].name
        );

        strToHigherCase(&tmp_str);

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    // Add import all tasks
    for(l_index = 0; l_index < im_c; l_index++) {
        tmp_str = (char*) calloc (
            strlen(imports[l_index].all_task) + 2,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            " %s",
            imports[l_index].all_task
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    fwrite("\n", sizeof(char), 1, file);

    // Write import all tasks
    for(l_index = 0; l_index < im_c; l_index++) {
        tmp_str = (char*) calloc (
            strlen(imports[l_index].all_task) + 2,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "%s:",
            imports[l_index].all_task
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);

        // Write all target requirements to all task
        for(r_index = 0; r_index < imports[l_index].task_c; r_index++) {
            tmp_str = (char*) calloc (
                strlen(imports[l_index].tasks[r_index].name) + 12,
                sizeof(char)
            );

            sprintf (
                tmp_str,
                " $(%s_TARGET)",
                imports[l_index].tasks[r_index].name
            );

            strToHigherCase(&tmp_str);

            fwrite (
                tmp_str,
                sizeof(char),
                strlen(tmp_str),
                file
            );

            free(tmp_str);
        }

        fwrite("\n", sizeof(char), 1, file);
    }
}


/* Write copy task if it exists */
static void sauWriteCopyTask (
    CopyInfo *p_cpy_info,
    PlatformInfo pi,
    char *pr_name
) {
    int32_t l_index;
    char *tmp_str;

    char **cpy_dsts = NULL;
    int32_t dst_c = 0;
    char **cpy_srcs = NULL;
    int32_t src_c = 0;

    // Verify copy task existance and 
    switch (pi)
    {
    case PLATFORM_APPLE:
        if
        (
            !p_cpy_info->all.cpy_c &&
            !p_cpy_info->apple_i.cpy_c
        ) return;

        cmbStrArr (
            p_cpy_info->all.dsts,
            p_cpy_info->all.cpy_c,
            p_cpy_info->apple_i.dsts,
            p_cpy_info->apple_i.cpy_c,
            &cpy_dsts,
            &dst_c
        );

        cmbStrArr (
            p_cpy_info->all.srcs,
            p_cpy_info->all.cpy_c,
            p_cpy_info->apple_i.srcs,
            p_cpy_info->apple_i.cpy_c,
            &cpy_srcs,
            &src_c
        );
        
        break;
    
    case PLATFORM_LINUX:
        if
        (
            !p_cpy_info->all.cpy_c &&
            !p_cpy_info->linux_i.cpy_c
        ) return;

        cmbStrArr (
            p_cpy_info->all.dsts,
            p_cpy_info->all.cpy_c,
            p_cpy_info->linux_i.dsts,
            p_cpy_info->linux_i.cpy_c,
            &cpy_dsts,
            &dst_c
        );

        cmbStrArr (
            p_cpy_info->all.srcs,
            p_cpy_info->all.cpy_c,
            p_cpy_info->linux_i.srcs,
            p_cpy_info->linux_i.cpy_c,
            &cpy_srcs,
            &src_c
        );

        break;

    case PLATFORM_WINDOWS:
        if 
        (
            !p_cpy_info->all.cpy_c &&
            !p_cpy_info->win_i.cpy_c
        ) return;

        cmbStrArr (
            p_cpy_info->all.dsts,
            p_cpy_info->all.cpy_c,
            p_cpy_info->win_i.dsts,
            p_cpy_info->win_i.cpy_c,
            &cpy_dsts,
            &dst_c
        );

        cmbStrArr (
            p_cpy_info->all.srcs,
            p_cpy_info->all.cpy_c,
            p_cpy_info->win_i.srcs,
            p_cpy_info->win_i.cpy_c,
            &cpy_srcs,
            &src_c
        );

        break;
    
    default:
        break;
    }

    is_cpy = true;
    fwrite (
        "\n" \
        CPY_TASK \
        ": ." \
        SCRIPT_NAME \
        "\n", 
        sizeof(char), 
        strlen(CPY_TASK) + strlen(SCRIPT_NAME) + 5, 
        file
    );
    
    // Write the copy task
    switch (pi)
    {
    case PLATFORM_WINDOWS:
        for(l_index = 0; l_index < src_c; l_index++) {
            tmp_str = (char*) calloc (
                strlen(cpy_srcs[l_index]) + strlen(cpy_dsts[l_index]) + 
                strlen(BUILD_DIR_VAR) + strlen(pr_name) + 21,
                sizeof(char)
            );

            sauFixWindowsPaths(&cpy_srcs[l_index], 1);

            sprintf (
                tmp_str,
                "\tXcopy /E /I %s $(%s)\\%s\\%s\n",
                cpy_srcs[l_index],
                BUILD_DIR_VAR,
                pr_name,
                cpy_dsts[l_index]
            );

            fwrite (
                tmp_str,
                sizeof(char),
                strlen(tmp_str),
                file
            );

            free(tmp_str);
        }
        break;

    case PLATFORM_APPLE:
    case PLATFORM_LINUX:
        for(l_index = 0; l_index < src_c; l_index++) {
            tmp_str = (char*) calloc (
                strlen(cpy_srcs[l_index]) + strlen(cpy_dsts[l_index]) + 
                strlen(BUILD_DIR_VAR) + strlen(pr_name) + 15,
                sizeof(char)
            );

            sprintf (
                tmp_str,
                "\tcp -r %s $(%s)/%s/%s\n",
                cpy_srcs[l_index],
                BUILD_DIR_VAR,
                pr_name,
                cpy_dsts[l_index]
            );

            fwrite (
                tmp_str,
                sizeof(char),
                strlen(tmp_str),
                file
            );

            free(tmp_str);
        }
        break;
    
    default:
        break;
    }
}


/* Write task compilation steps into makefile */
static void sauWriteTask (
    TaskInfo *p_task,
    CompilerFlagInfos *p_fi,
    PlatformInfo pi,
    char *src_var
) {
    char *target, *objs;
    int32_t c_src_c = 0;
    int32_t cxx_src_c = 0;

    char **obj_arr;
    char **src_arr;
    int32_t src_c;

    // Count the amount of source files for C++ and C
    sauCountSrcTypes (
        p_task->src_info.all.files,
        p_task->src_info.all.file_c,
        &c_src_c,
        &cxx_src_c
    );
    
    switch (pi)
    {
    case PLATFORM_APPLE:
        sauCountSrcTypes (
            p_task->src_info.apple_i.files,
            p_task->src_info.apple_i.file_c,
            &c_src_c,
            &cxx_src_c
        );

        // Combine all sources into one array
        cmbStrArr (
            p_task->src_info.all.files,
            p_task->src_info.all.file_c,
            p_task->src_info.apple_i.files,
            p_task->src_info.apple_i.file_c,
            &src_arr,
            &src_c
        );

        // Combine all object file paths into one array
        cmbStrArr (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.apple_i.files,
            p_task->obj_info.apple_i.file_c,
            &obj_arr,
            &src_c
        );
        break;

    case PLATFORM_LINUX:
        sauCountSrcTypes (
            p_task->src_info.linux_i.files,
            p_task->src_info.linux_i.file_c,
            &c_src_c,
            &cxx_src_c
        );

        // Combine all sources into one array
        cmbStrArr (
            p_task->src_info.all.files,
            p_task->src_info.all.file_c,
            p_task->src_info.linux_i.files,
            p_task->src_info.linux_i.file_c,
            &src_arr,
            &src_c
        );

        // Combine all object file paths into one array
        cmbStrArr (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.linux_i.files,
            p_task->obj_info.linux_i.file_c,
            &obj_arr,
            &src_c
        );
        break;    
    
    case PLATFORM_WINDOWS:
        sauCountSrcTypes (
            p_task->src_info.win_i.files,
            p_task->src_info.win_i.file_c,
            &c_src_c,
            &cxx_src_c
        );

        // Combine all sources into one array
        cmbStrArr (
            p_task->src_info.all.files,
            p_task->src_info.all.file_c,
            p_task->src_info.win_i.files,
            p_task->src_info.win_i.file_c,
            &src_arr,
            &src_c
        );

        // Combine all object file paths into one array
        cmbStrArr (
            p_task->obj_info.all.files,
            p_task->obj_info.all.file_c,
            p_task->obj_info.win_i.files,
            p_task->obj_info.win_i.file_c,
            &obj_arr,
            &src_c
        );
        break;
    
    default:
        break;
    }
    
    // Get the target variable
    target = (char*) calloc (
        strlen(p_task->name) + 8,
        sizeof(char)
    );
    
    sprintf (
        target,
        "%s_TARGET",
        p_task->name
    );

    strToHigherCase(&target);

    // Get the objects variable
    objs = (char*) calloc (
        strlen(p_task->name) + 5,
        sizeof(char)
    );

    sprintf (
        objs,
        "%s_OBJ",
        p_task->name
    );

    strToHigherCase(&objs);
    
    // Write main task
    sauWriteMainTask (
        p_task,
        p_fi,
        c_src_c,
        cxx_src_c,
        target,
        objs,
        pi
    );

    int32_t l_index;
    // Write all subtasks for every object
    for(l_index = 0; l_index < src_c; l_index++)
        sauWriteSubtask (
            obj_arr[l_index], 
            src_arr[l_index], 
            p_task->type, 
            p_fi,
            src_var,
            pi 
        );

    free(target);
    free(objs);
}


/* Write clean task to makefile */
static void sauWriteClean (
    TaskInfo *tasks,
    int32_t task_c,
    PlatformInfo pi,
    char *pr_name
) {
    char *tmp_str;

    // Write the clean task declaration
    fwrite (
        ".PHONY: clean\n" \
        "clean:\n",
        sizeof(char),
        21,
        file
    );

    int32_t l_index;
    switch (pi)
    {
    case PLATFORM_APPLE:
    case PLATFORM_LINUX:
        tmp_str = (char*) calloc (
            strlen(BUILD_DIR_VAR) + strlen(pr_name) + 16,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\trm -rf $(%s)/%s/*\n",
            BUILD_DIR_VAR,
            pr_name
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);

        for(l_index = 0; l_index < task_c; l_index++) {
            tmp_str = (char*) calloc (
                18 + strlen(BUILD_DIR_VAR) + strlen(tasks[l_index].name),
                sizeof(char)
            );
            
            sprintf (
                tmp_str,
                "\trm $(%s)/obj/%s/*.o\n",
                BUILD_DIR_VAR,
                tasks[l_index].name
            );

            fwrite (
                tmp_str,
                sizeof(char),
                strlen(tmp_str),
                file
            );

            free(tmp_str);
        }   
        break;

    case PLATFORM_WINDOWS:
        tmp_str = (char*) calloc (
            2 * (strlen(BUILD_DIR_VAR) + strlen(pr_name)) + 60,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\tdel /s /q $(%s)\\%s\\*\n" \
            "\tfor /d %%x in ($(%s)\\%s\\*) do @rd /s /q \"%%x\"\n",
            BUILD_DIR_VAR,
            pr_name,
            BUILD_DIR_VAR,
            pr_name
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);

        for(l_index = 0; l_index < task_c; l_index++) {
            tmp_str = (char*) calloc (
                19 + strlen(BUILD_DIR_VAR) + strlen(tasks[l_index].name),
                sizeof(char)
            );

            sprintf (
                tmp_str,
                "\tdel $(%s)\\obj\\%s\\*.o\n",
                BUILD_DIR_VAR,
                tasks[l_index].name
            );

            fwrite (
                tmp_str,
                sizeof(char),
                strlen(tmp_str),
                file
            );

            free(tmp_str);
        }
        break;
    
    default:
        break;
    }
}


/* Find all object file names */
static void sauGetObjectFileNames(TaskInfo *p_task) {
    // Non platform specific
    sauGetPlatformSpecificObjNames (
        p_task->src_info.all.files,
        p_task->src_info.all.file_c,
        &p_task->obj_info.all.files,
        &p_task->obj_info.all.file_c,
        p_task->name
    );

    // MacOS specific
    sauGetPlatformSpecificObjNames (
        p_task->src_info.apple_i.files,
        p_task->src_info.apple_i.file_c,
        &p_task->obj_info.apple_i.files,
        &p_task->obj_info.apple_i.file_c,
        p_task->name
    );

    // Linux specific
    sauGetPlatformSpecificObjNames (
        p_task->src_info.linux_i.files,
        p_task->src_info.linux_i.file_c,
        &p_task->obj_info.linux_i.files,
        &p_task->obj_info.linux_i.file_c,
        p_task->name
    );

    // Windows specific
    sauGetPlatformSpecificObjNames (
        p_task->src_info.win_i.files,
        p_task->src_info.win_i.file_c,
        &p_task->obj_info.win_i.files,
        &p_task->obj_info.win_i.file_c,
        p_task->name
    );
}


/* Initialise all makefile variables */
void sauInitMakefileVars (
    BuildInfo *p_bi,
    BuildInfo *imports,
    int32_t im_c
) {
    int32_t l_index, r_index;
    lib_targets = (char**) calloc (
        1,
        sizeof(char*)
    );
    lib_tar_c = 0;

    file = fopen("Makefile", "w");

    // Write all beginning makefile comments
    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        fwrite (
            APPLE_PLATFORM_COMMENT,
            sizeof(char),
            strlen(APPLE_PLATFORM_COMMENT),
            file
        );
        break;

    case PLATFORM_LINUX:
        fwrite (
            LINUX_PLATFORM_COMMENT,
            sizeof(char),
            strlen(LINUX_PLATFORM_COMMENT),
            file
        );
        break;

    case PLATFORM_WINDOWS:
        fwrite (
            WINDOWS_PLATFORM_COMMENT,
            sizeof(char),
            strlen(WINDOWS_PLATFORM_COMMENT),
            file
        );
        break;
    
    default:
        break;
    }
    
    fwrite (
        START_COMMENT,
        sizeof(char),
        strlen(START_COMMENT),
        file
    );

    fwrite (
        MOD_COMMENT,
        sizeof(char),
        strlen(MOD_COMMENT),
        file
    );

    fwrite (
        GIT_PAGE_COMMENT,
        sizeof(char),
        strlen(GIT_PAGE_COMMENT),
        file
    );

    sauFindLibTargets(p_bi);
    
    // Write C compiler variable
    if(p_bi->premake.cc_info.cc)
        sauWriteSingleLineVar (
            C_COMPILER_VAR, 
            p_bi->premake.cc_info.cc
        );

    // Write C++ compiler variable
    if(p_bi->premake.cc_info.cxx)
        sauWriteSingleLineVar (
            CPP_COMPILER_VAR, 
            p_bi->premake.cc_info.cxx
        );

    fwrite("\n", sizeof(char), 1, file);

    /********** Imported project *********/
    // Write premake information
    for(l_index = 0; l_index < im_c; l_index++) {
        sauWritePremake(&imports[l_index], true);
        
        // Write all task related variables
        for(r_index = 0; r_index < imports[l_index].task_c; r_index++) {
            fwrite("\n", sizeof(char), 1, file);
            sauGetObjectFileNames(&imports[l_index].tasks[r_index]);
            sauWriteTaskTargetVar (
                p_bi->platform, 
                &imports[l_index].tasks[r_index], 
                p_bi->premake.project_name
            );
            sauWriteTaskObjVar(p_bi->platform, &imports[l_index].tasks[r_index]);
        }
    }

    /****** Master project ********/
    sauWritePremake(p_bi, false);
    // Write all task related variables
    for(r_index = 0; r_index < p_bi->task_c; r_index++) {
        fwrite("\n", sizeof(char), 1, file);
        sauGetObjectFileNames(&p_bi->tasks[r_index]);
        sauWriteTaskTargetVar (
            p_bi->platform, 
            &p_bi->tasks[r_index], 
            p_bi->premake.project_name
        );
        sauWriteTaskObjVar(p_bi->platform, &p_bi->tasks[r_index]);
    }
}


/* Main Makefile creator function */
void sauWriteMakefile (
    BuildInfo *p_bi, 
    BuildInfo *imports, 
    int32_t im_c
) {
    int32_t l_index, r_index;
    char *tmp_str;

    // Write init script into makefile
    if(p_bi->platform == PLATFORM_WINDOWS) {
        smWriteBatchInit (
            p_bi,
            imports,
            im_c
        );
    }

    else {
        smWriteBashInit (
            p_bi,
            imports,
            im_c
        );
    }

    // Write init task into makefile
    fwrite("\n", sizeof(char), 1, file);
    sauWriteInitTask(p_bi->platform);

    // Write copy and link tasks if they exist
    sauWriteCopyTask (
        &p_bi->cpy_info, 
        p_bi->platform, 
        p_bi->premake.project_name 
    );
    
    // Write default all task
    fwrite("\n", sizeof(char), 1, file);
    sauWriteAllTask (
        p_bi->tasks, 
        p_bi->task_c, 
        imports, 
        im_c
    );

    // Write import tasks into makefile
    for(l_index = 0; l_index < im_c; l_index++) {
        // Write import build comment
        tmp_str = (char*) calloc (
            42 + strlen(imports[l_index].premake.project_name),
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\n\n########### IMPORT BUILD: %s ###########\n",
            imports[l_index].premake.project_name
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);

        for(r_index = 0; r_index < imports[l_index].task_c; r_index++) {
            fwrite("\n", sizeof(char), 1, file);
            
            // Find src variable
            tmp_str = sauMakeVarName (
                SRC_DIR_VAR, 
                imports[l_index].premake.project_name,
                true
            );

            sauWriteTask (
                &imports[l_index].tasks[r_index],
                &imports->flag_infos,
                p_bi->platform,
                tmp_str
            );

            free(tmp_str);
        }

        // Declare end of import build
        tmp_str = (char*) calloc (
            46,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "\n########### END OF IMPORT BUILD ###########\n"
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }


    // Write main build comment
    tmp_str = (char*) calloc (
        40 + strlen(p_bi->premake.project_name),
        sizeof(char)
    );

    sprintf (
        tmp_str,
        "\n\n########### MAIN BUILD: %s ###########\n",
        p_bi->premake.project_name
    );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );
    
    free(tmp_str);

    for(l_index = 0; l_index < p_bi->task_c; l_index++) {
        fwrite("\n", sizeof(char), 1, file);
        sauWriteTask (
            &p_bi->tasks[l_index],
            &p_bi->flag_infos, 
            p_bi->platform,
            SRC_DIR_VAR
        );
    }

    // Declare end of main build
    tmp_str = (char*) calloc (
        44,
        sizeof(char)
    );

    sprintf (
        tmp_str,
        "\n########### END OF MAIN BUILD ###########\n"
    );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );
    
    free(tmp_str);

    // Write phony tasks
    fwrite("\n", sizeof(char), 1, file);
    sauWriteClean (
        p_bi->tasks, 
        p_bi->task_c, 
        p_bi->platform, 
        p_bi->premake.project_name
    );

    fclose(file);
}