#define BUILD_MAKER_PRIVATE
#include "saucer.h"

static uint8_t is_cpp   = false;
static uint8_t is_c     = false;
static int32_t base_ws  = 0;

/* Find the source file extension */
static void sauFindSrcFileExt(char *file) {
    int32_t l_index, ch_index;
    int32_t beg_index;
    int32_t ext_ch_c = 3;
    char *ext = (char*) calloc ( 
        ext_ch_c + 1,
        sizeof(char)
    );

    // Find the file extension beginning index
    for(l_index = strlen(file) - 1; l_index >= 0; l_index--) {
        if(file[l_index] == 0x2E) {
            beg_index = l_index + 1;
            break;
        } 
    }

    for 
    (
        l_index = beg_index,
        ch_index = 0; 
        l_index < strlen(file) && 
        ch_index < ext_ch_c; 
        l_index++,
        ch_index++
    ) ext[ch_index] = file[l_index];

    // Check the file extension for possible source file extensions
    if(!strcmp(ext, "c")) is_c = true;
    else if
    (
        !strcmp(ext, "cc") ||
        !strcmp(ext, "cxx") ||
        !strcmp(ext, "cpp")
    ) is_cpp = true;
    else {
        printf("EXT: %s\n", ext);
        BERR (
            "unknown source file ",
            file
        );
        exit(ERRC_BUILD_CFG);
    }
} 


/* Find all linux specific values */
static void sauFindPlatformValues (
    KeyData *keys, 
    int32_t key_c, 
    int32_t cur_index, 
    char ***p_vals,
    int32_t *p_val_c,
    char *platform
) {
    int32_t l_index; 
    int32_t max_ws = keys[cur_index].ws_c;

    // Find the platform key
    for(l_index = cur_index + 1; l_index < key_c; l_index++) {
        if(keys[l_index].ws_c <= max_ws) return;
        if
        (
            !strcmp (
                keys[l_index].key_name, 
                platform
            )
        ) {
            (*p_vals) = keys[l_index].key_vals;
            (*p_val_c) = keys[l_index].key_val_c;
            return;
        };
    } 
}


/* Push string to dynamic string array */
static void sauPushToStrArr (
    char ***p_dst_arr, 
    int32_t *p_arr_len,
    char *val
) {
    (*p_arr_len)++;
    
    char **tmp_arr = (char**) realloc (
        (*p_dst_arr),
        (*p_arr_len) * sizeof(char*)  
    );

    // Check for reallocation error
    if(!tmp_arr) {
        MEMERR("failed to reallocate memory");
        exit(ERRC_MEM);
    }

    (*p_dst_arr) = tmp_arr;
    (*p_dst_arr)[(*p_arr_len) - 1] = val; 
}


/* Verify that the base values are specified in build config */
static void sauVerifyPremake(BuildInfo *p_bi) {
    // Check if project name is specified
    if(!p_bi->premake.project_name) {
        BERR("no project name specified", "");
        exit(ERRC_BUILD_CFG);
    }

    // Check if source directory is specified
    if(!p_bi->premake.src_dir) {
        BERR("no source directory specified", "");
        exit(ERRC_BUILD_CFG);
    }
    
    // Check if build directory is specified
    if(!p_bi->premake.build_dir) {
        BERR("no build directory specified", "");
        exit(ERRC_BUILD_CFG);
    }
}


/* Set all the premake config values to null */
static void sauInitPremakeValues(BuildInfo *p_bi) {
    // Assign current build platform
    #ifdef __linux
        p_bi->platform = PLATFORM_LINUX;
    #endif
    #ifdef _WIN32
        p_bi->platform = PLATFORM_WINDOWS;
    #endif
    #ifdef __APPLE__
        p_bi->platform = PLATFORM_APPLE;
    #endif

    // Compiler
    p_bi->premake.cc_info.cc = NULL;
    p_bi->premake.cc_info.cxx = NULL;
    
    // CC flags
    p_bi->premake.cc_flags.all.flags = NULL;
    p_bi->premake.cc_flags.all.flag_c = 0;
    p_bi->premake.cc_flags.apple_i.flags = NULL;
    p_bi->premake.cc_flags.apple_i.flag_c = 0;
    p_bi->premake.cc_flags.linux_i.flags = NULL;
    p_bi->premake.cc_flags.linux_i.flag_c = 0;
    p_bi->premake.cc_flags.win_i.flags = NULL;
    p_bi->premake.cc_flags.win_i.flag_c = 0;
    
    // CXX flags
    p_bi->premake.cxx_flags.all.flags = NULL;
    p_bi->premake.cxx_flags.all.flag_c = 0;
    p_bi->premake.cxx_flags.apple_i.flags = NULL;
    p_bi->premake.cxx_flags.apple_i.flag_c = 0;
    p_bi->premake.cxx_flags.linux_i.flags = NULL;
    p_bi->premake.cxx_flags.linux_i.flag_c = 0;
    p_bi->premake.cxx_flags.win_i.flags = NULL;
    p_bi->premake.cxx_flags.win_i.flag_c = 0;

    // Include flags
    p_bi->premake.incl_info.all.paths = NULL;
    p_bi->premake.incl_info.all.path_c = 0;
    p_bi->premake.incl_info.apple_i.paths = NULL;
    p_bi->premake.incl_info.apple_i.path_c = 0;
    p_bi->premake.incl_info.linux_i.paths = NULL;
    p_bi->premake.incl_info.linux_i.path_c = 0;
    p_bi->premake.incl_info.win_i.paths = NULL;
    p_bi->premake.incl_info.win_i.path_c = 0;

    // Library paths
    p_bi->premake.lib_info.all.paths = NULL;
    p_bi->premake.lib_info.all.path_c = 0;
    p_bi->premake.lib_info.apple_i.paths = NULL;
    p_bi->premake.lib_info.apple_i.path_c = 0;
    p_bi->premake.lib_info.linux_i.paths = NULL;
    p_bi->premake.lib_info.linux_i.path_c = 0;
    p_bi->premake.lib_info.win_i.paths = NULL;
    p_bi->premake.lib_info.win_i.path_c = 0;

    // Project specifications
    p_bi->premake.src_dir = NULL;
    p_bi->premake.build_dir = NULL;
    p_bi->premake.project_name = NULL;

    // Task specifications
    p_bi->task_c = 0;
    p_bi->tasks = (TaskInfo*) calloc (
        1, 
        sizeof(TaskInfo)
    );

    // Copies
    p_bi->cpy_info.all.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.all.dsts = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.all.cpy_c = 0;
    
    p_bi->cpy_info.apple_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.apple_i.dsts = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.apple_i.cpy_c = 0;
    
    p_bi->cpy_info.linux_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.linux_i.dsts = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.linux_i.cpy_c = 0;
    
    p_bi->cpy_info.win_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.win_i.dsts = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->cpy_info.win_i.cpy_c = 0;

    // Links
    p_bi->link_info.all.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.all.links = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.all.link_c = 0;
    
    p_bi->link_info.apple_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.apple_i.links = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.apple_i.link_c = 0;
    
    p_bi->link_info.linux_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.linux_i.links = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.linux_i.link_c = 0;
    
    p_bi->link_info.win_i.srcs = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.win_i.links = (char**) calloc (
        1,
        sizeof(char*)
    );
    p_bi->link_info.win_i.link_c = 0;
}


/* Find all premake values */
static void sauFindPremakeValues (
    KeyData *keys, 
    int32_t pre_beg_index, 
    int32_t pre_end_index, 
    BuildInfo *p_bi
) {
    int32_t l_index;
    
    for(l_index = pre_beg_index; l_index < pre_end_index; l_index++) {
        // Check all possible values for subkeys
        if(!strcmp(keys[l_index].key_name, "cc"))
            p_bi->premake.cc_info.cc = *keys[l_index].key_vals;
        
        else if(!strcmp(keys[l_index].key_name, "cxx"))
            p_bi->premake.cc_info.cxx = *keys[l_index].key_vals;

        else if(!strcmp(keys[l_index].key_name, "cc_flags")) {
            p_bi->premake.cc_flags.all.flags = keys[l_index].key_vals;
            p_bi->premake.cc_flags.all.flag_c = keys[l_index].key_val_c;
            
            sauFindPlatformValues (
                keys, 
                pre_end_index,
                l_index,
                &p_bi->premake.cc_flags.linux_i.flags,
                &p_bi->premake.cc_flags.linux_i.flag_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.cc_flags.apple_i.flags,
                &p_bi->premake.cc_flags.apple_i.flag_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.cc_flags.win_i.flags,
                &p_bi->premake.cc_flags.win_i.flag_c,
                "windows"
            );
        }

        else if(!strcmp(keys[l_index].key_name, "cxx_flags")) {
            p_bi->premake.cxx_flags.all.flags = keys[l_index].key_vals;
            p_bi->premake.cxx_flags.all.flag_c = keys[l_index].key_val_c;

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.cxx_flags.linux_i.flags,
                &p_bi->premake.cxx_flags.linux_i.flag_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.cxx_flags.apple_i.flags,
                &p_bi->premake.cxx_flags.apple_i.flag_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.cxx_flags.win_i.flags,
                &p_bi->premake.cxx_flags.win_i.flag_c,
                "windows"
            );
        }

        else if(!strcmp(keys[l_index].key_name, "include_path")) {
            p_bi->premake.incl_info.all.paths = keys[l_index].key_vals;
            p_bi->premake.incl_info.all.path_c = keys[l_index].key_val_c;

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.incl_info.linux_i.paths,
                &p_bi->premake.incl_info.linux_i.path_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.incl_info.apple_i.paths,
                &p_bi->premake.incl_info.apple_i.path_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.incl_info.win_i.paths,
                &p_bi->premake.incl_info.win_i.path_c,
                "windows"
            );
        }

        else if(!strcmp(keys[l_index].key_name, "library_path")) {
            p_bi->premake.lib_info.all.paths = keys[l_index].key_vals;
            p_bi->premake.lib_info.all.path_c = keys[l_index].key_val_c;

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.lib_info.linux_i.paths,
                &p_bi->premake.lib_info.linux_i.path_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.lib_info.apple_i.paths,
                &p_bi->premake.lib_info.apple_i.path_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                pre_end_index,
                l_index,
                &p_bi->premake.lib_info.win_i.paths,
                &p_bi->premake.lib_info.win_i.path_c,
                "windows"
            );
        }

        else if(!strcmp(keys[l_index].key_name, "src_dir")) {
            // Check for build config error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "no source directory specified", 
                    ""
                );
                exit(ERRC_BUILD_CFG);
            }

            p_bi->premake.src_dir = *keys[l_index].key_vals;

            if(p_bi->premake.src_dir[strlen(p_bi->premake.src_dir) - 1] == 0x2F)
                p_bi->premake.src_dir[strlen(p_bi->premake.src_dir) - 1] = 0x00;
        }

        else if(!strcmp(keys[l_index].key_name, "build_dir")) {
            // Check for build config error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "no build directory specified", 
                    ""
                );
                exit(ERRC_BUILD_CFG);
            }

            p_bi->premake.build_dir = *keys[l_index].key_vals;

            if(p_bi->premake.build_dir[strlen(p_bi->premake.build_dir) - 1] == 0x2F)
                p_bi->premake.build_dir[strlen(p_bi->premake.build_dir) - 1] = 0x00;
        }
        
        else if(!strcmp(keys[l_index].key_name, "project_name")) {
            // Check for build config error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "no project name specified", 
                    ""
                );
                exit(ERRC_BUILD_CFG);
            }

            p_bi->premake.project_name = *keys[l_index].key_vals;
        }
    }   
}


/* Find all information from task */
static void sauGetTaskData (
    KeyData *keys,
    int32_t key_c,
    int32_t key_index,
    BuildInfo *p_bi
) {
    int32_t l_index, r_index;
    // Reallocate memory for taskinfo
    p_bi->task_c++;
    TaskInfo *p_ti = (TaskInfo*) realloc (
        p_bi->tasks,
        p_bi->task_c * sizeof(TaskInfo)
    );

    if(!p_ti) {
        MEMERR("failed to reallocate memory for taskinfo");
        exit(ERRC_MEM);
    }

    p_bi->tasks = p_ti;
    p_bi->tasks[p_bi->task_c - 1].name = keys[key_index].key_name;
    
    p_bi->tasks[p_bi->task_c - 1].src_info.all.files = NULL;
    p_bi->tasks[p_bi->task_c - 1].src_info.all.file_c = 0;
    p_bi->tasks[p_bi->task_c - 1].src_info.apple_i.files = NULL;
    p_bi->tasks[p_bi->task_c - 1].src_info.apple_i.file_c = 0;
    p_bi->tasks[p_bi->task_c - 1].src_info.linux_i.files = NULL;
    p_bi->tasks[p_bi->task_c - 1].src_info.linux_i.file_c = 0;
    p_bi->tasks[p_bi->task_c - 1].src_info.win_i.files = NULL;
    p_bi->tasks[p_bi->task_c - 1].src_info.win_i.file_c = 0;
    
    p_bi->tasks[p_bi->task_c - 1].deps_info.all.deps = NULL;
    p_bi->tasks[p_bi->task_c - 1].deps_info.all.deps_c = 0;
    p_bi->tasks[p_bi->task_c - 1].deps_info.apple_i.deps = NULL;
    p_bi->tasks[p_bi->task_c - 1].deps_info.apple_i.deps_c = 0;
    p_bi->tasks[p_bi->task_c - 1].deps_info.linux_i.deps = NULL;
    p_bi->tasks[p_bi->task_c - 1].deps_info.linux_i.deps_c = 0;
    p_bi->tasks[p_bi->task_c - 1].deps_info.win_i.deps = NULL;
    p_bi->tasks[p_bi->task_c - 1].deps_info.win_i.deps_c = 0;

    // Check for build config error
    if(key_index + 1 >= key_c) {
        BERR (
            "empty task specification for ", 
            p_bi->tasks[p_bi->task_c - 1].name    
        );
        exit(ERRC_BUILD_CFG);
    }
    
    // Find all task type specification value
    for(l_index = key_index + 1; l_index < key_c; l_index++) {
        if(keys[l_index].ws_c <= keys[key_index].ws_c) break;
        // Task type
        if(!strcmp(keys[l_index].key_name, "type")) {
            // Check for build config error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "no type specified for ",
                    p_bi->tasks[p_bi->task_c - 1].name
                ); 
                exit(ERRC_BUILD_CFG);
            }

            // Find the task type
            // Dynamic library
            if(!strcmp(*keys[l_index].key_vals, "dynamic_lib")) 
                p_bi->tasks[p_bi->task_c - 1].type = TARGET_TYPE_DYNAMIC_LIBRARY;
            // Static library
            else if(!strcmp(*keys[l_index].key_vals, "static_lib"))
                p_bi->tasks[p_bi->task_c - 1].type = TARGET_TYPE_STATIC_LIBRARY;
            // Executable
            else if(!strcmp(*keys[l_index].key_vals, "exec"))
                p_bi->tasks[p_bi->task_c - 1].type = TARGET_TYPE_EXECUTABLE;
            // Unknown (build error)
            else {
                BERR (
                    "unknown task type for ",
                    p_bi->tasks[p_bi->task_c - 1].name
                );
                exit(ERRC_BUILD_CFG);
            }
        }

        // Find task source files
        else if(!strcmp(keys[l_index].key_name, "src")) {
            // Check for build config error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "no source files specified for task ",
                    keys[l_index].key_name    
                );

                exit(ERRC_BUILD_CFG);
            }

            p_bi->tasks[p_bi->task_c - 1].src_info.all.files = keys[l_index].key_vals;
            p_bi->tasks[p_bi->task_c - 1].src_info.all.file_c = keys[l_index].key_val_c;
            
            // Find platform specific source files
            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].src_info.apple_i.files,
                &p_bi->tasks[p_bi->task_c - 1].src_info.apple_i.file_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].src_info.linux_i.files,
                &p_bi->tasks[p_bi->task_c - 1].src_info.linux_i.file_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].src_info.win_i.files,
                &p_bi->tasks[p_bi->task_c - 1].src_info.win_i.file_c,
                "windows"
            );

            // Find file extensions, if needed
            for(r_index = 0; r_index < keys[l_index].key_val_c; r_index++) {
                if (
                    !is_c || 
                    !is_cpp
                ) sauFindSrcFileExt(keys[l_index].key_vals[r_index]);

                else break;
            }
        }

        // Find task dependencies (optional)
        else if
        (
            !strcmp(keys[l_index].key_name, "deps") &&
            keys[l_index].key_val_c
        ) {
            p_bi->tasks[p_bi->task_c - 1].deps_info.all.deps = keys[l_index].key_vals;
            p_bi->tasks[p_bi->task_c - 1].deps_info.all.deps_c = keys[l_index].key_val_c;

            // Find platform specific values
            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.apple_i.deps,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.apple_i.deps_c,
                "apple"
            );

            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.linux_i.deps,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.linux_i.deps_c,
                "linux"
            );

            sauFindPlatformValues (
                keys,
                key_c,
                l_index,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.win_i.deps,
                &p_bi->tasks[p_bi->task_c - 1].deps_info.win_i.deps_c,
                "windows"
            );
        }
    }
}


/* Create premake configuration into buildinfo */
static void sauAssemblePremake (
    KeyData *keys, 
    int32_t key_c, 
    BuildInfo *p_bi
) {
    int32_t l_index;
    uint8_t is_premake = false;
    int32_t pre_beg_index, pre_end_index;
    // Find premake key beginning index
    for(l_index = 0; l_index < key_c; l_index++) {
        if
        (
            !strcmp(keys[l_index].key_name, "premake") &&
            keys[l_index].ws_c == base_ws
        ) {
            pre_beg_index = l_index;
            is_premake = true;
            break;
        }
    }

    // Check for build config error
    if(!is_premake || pre_beg_index == key_c - 1) {
        BERR (
            "no premake configs specified", 
            ""
        );
        exit(ERRC_BUILD_CFG);
    }
    
    // Find premake key ending index
    for(l_index = pre_beg_index + 1; l_index < key_c; l_index++) {
        if(keys[l_index].ws_c <= keys[pre_beg_index].ws_c) {
            pre_end_index = l_index;
            break;
        }
    }

    sauFindPremakeValues (
        keys, 
        pre_beg_index, 
        pre_end_index, 
        p_bi
    );

    // Extra overhead for verifying that important variables are specified
    sauVerifyPremake(p_bi);
}


/* Assemble all information needed for tasks */
static void sauAssembleTasks 
(
    KeyData *keys, 
    int32_t key_c, 
    BuildInfo *p_bi
) {
    int32_t l_index;

    // Find the tasks key
    uint8_t found_tasks;
    int32_t task_beg, max_ws;
    for(l_index = 0; l_index < key_c; l_index++) {
        if
        (
            !strcmp(keys[l_index].key_name, "tasks") &&
            base_ws == keys[l_index].ws_c
        ) {
            task_beg = l_index;
            found_tasks = true;
            break;
        }
    }

    // Check for build error
    if(!found_tasks || task_beg == key_c - 1) {
        BERR (
            "no tasks specified",
            ""
        );
        exit(ERRC_BUILD_CFG);
    }

    max_ws = keys[task_beg + 1].ws_c;
    // Find all tasks
    for(l_index = task_beg + 1; l_index < key_c; l_index++) {
        // Found task
        if
        (
            keys[l_index].ws_c > base_ws &&
            keys[l_index].ws_c <= max_ws
        ) sauGetTaskData(keys, key_c, l_index, p_bi);
        else if(keys[l_index].ws_c <= base_ws) break;
    }
}


static void sauFindLinks (
    KeyData *keys,
    int32_t key_c,
    BuildInfo *p_bi
) {
    int32_t l_index;
    
    // Find the link key
    uint8_t found_link = false;
    int32_t max_ws, beg_index;
    for(l_index = 0; l_index < key_c; l_index++) {
        if
        (
            !strcmp(keys[l_index].key_name, "link") &&
            base_ws == keys[l_index].ws_c
        ) {
            beg_index = l_index;
            found_link = true;
            break;
        }
    }

    if(!found_link) return;

    // Check for build error
    if(beg_index == key_c - 1) {
        BERR (
            "link key with no values specified",
            ""
        );

        exit(ERRC_BUILD_CFG);
    }

    max_ws = keys[beg_index + 1].ws_c;

    PlatformInfo pi = PLATFORM_ALL;
    int32_t pl_min_ws;
    uint8_t is_platform_val = false;

    for(l_index = beg_index + 1; l_index < key_c; l_index++) {
        if
        (   
            !is_platform_val &&
            keys[l_index].ws_c > base_ws &&
            keys[l_index].ws_c <= max_ws
        ) {
            if(!strcmp(keys[l_index].key_name, "apple")) {
                is_platform_val = true;
                pi = PLATFORM_APPLE;
                pl_min_ws = keys[l_index].ws_c;
            }

            else if(!strcmp(keys[l_index].key_name, "linux")) {
                is_platform_val = true;
                pi = PLATFORM_LINUX;
                pl_min_ws = keys[l_index].ws_c;
            }

            else if(!strcmp(keys[l_index].key_name, "windows")) {
                is_platform_val = true;
                pi = PLATFORM_WINDOWS;
                pl_min_ws = keys[l_index].ws_c;
            }

            else {
                // Check for value error
                if(!keys[l_index].key_val_c) {
                    BERR (
                        "link key with no value",
                        keys[l_index].key_name
                    );

                    exit(ERRC_BUILD_CFG);
                }

                sauPushToStrArr (
                    &p_bi->link_info.all.links,
                    &p_bi->link_info.all.link_c,
                    keys[l_index].key_name
                );

                p_bi->link_info.all.link_c--;
                sauPushToStrArr (
                    &p_bi->link_info.all.srcs,
                    &p_bi->link_info.all.link_c,
                    *keys[l_index].key_vals
                );
            }
        }

        else if
        (
            is_platform_val &&
            keys[l_index].ws_c > pl_min_ws
        ) {
            // Check for value error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "link key with no value",
                    keys[l_index].key_name
                );

                exit(ERRC_BUILD_CFG);
            }

            switch (pi)
            {
            case PLATFORM_APPLE:
                sauPushToStrArr (
                    &p_bi->link_info.apple_i.links,
                    &p_bi->link_info.apple_i.link_c,
                    keys[l_index].key_name
                );

                p_bi->link_info.apple_i.link_c--;
                sauPushToStrArr (
                    &p_bi->link_info.apple_i.srcs,
                    &p_bi->link_info.apple_i.link_c,
                    *keys[l_index].key_vals
                );
                break;

            case PLATFORM_LINUX:
                sauPushToStrArr (
                    &p_bi->link_info.linux_i.links,
                    &p_bi->link_info.linux_i.link_c,
                    keys[l_index].key_name
                );

                p_bi->link_info.linux_i.link_c--;
                
                sauPushToStrArr (
                    &p_bi->link_info.linux_i.srcs,
                    &p_bi->link_info.linux_i.link_c,
                    *keys[l_index].key_vals
                );
                break;

            case PLATFORM_WINDOWS:
                sauPushToStrArr (
                    &p_bi->link_info.win_i.links,
                    &p_bi->link_info.win_i.link_c,
                    keys[l_index].key_name
                );

                p_bi->link_info.win_i.link_c--;
                sauPushToStrArr (
                    &p_bi->link_info.win_i.srcs,
                    &p_bi->link_info.win_i.link_c,
                    *keys[l_index].key_vals
                );
                break;
            
            default:
                break;
            }
        }

        else if 
        (
            is_platform_val &&
            keys[l_index].ws_c <= pl_min_ws &&
            keys[l_index].ws_c > base_ws
        )  {
            is_platform_val = false;
            l_index--;
        }

        else if(keys[l_index].ws_c <= base_ws) return;
    }
}


static void sauFindCopies (
    KeyData *keys, 
    int32_t key_c,
    BuildInfo *p_bi
) {
    int32_t l_index;
    
    // Find the link key
    uint8_t found_link = false;
    int32_t max_ws, beg_index;
    for(l_index = 0; l_index < key_c; l_index++) {
        if
        (
            !strcmp(keys[l_index].key_name, "cpy") &&
            base_ws == keys[l_index].ws_c
        ) {
            beg_index = l_index;
            found_link = true;
            break;
        }
    }

    if(!found_link) return;

    // Check for build error
    if(beg_index == key_c - 1) {
        BERR (
            "link key with no values specified",
            ""
        );

        exit(ERRC_BUILD_CFG);
    }

    max_ws = keys[beg_index + 1].ws_c;

    PlatformInfo pi = PLATFORM_ALL;
    int32_t pl_min_ws;
    uint8_t is_platform_val = false;

    for(l_index = beg_index + 1; l_index < key_c; l_index++) {
        if
        (   
            !is_platform_val &&
            keys[l_index].ws_c > base_ws &&
            keys[l_index].ws_c <= max_ws
        ) {
            if(!strcmp(keys[l_index].key_name, "apple")) {
                is_platform_val = true;
                pi = PLATFORM_APPLE;
                pl_min_ws = keys[l_index].ws_c;
            }

            else if(!strcmp(keys[l_index].key_name, "linux")) {
                is_platform_val = true;
                pi = PLATFORM_LINUX;
                pl_min_ws = keys[l_index].ws_c;
            }

            else if(!strcmp(keys[l_index].key_name, "windows")) {
                is_platform_val = true;
                pi = PLATFORM_WINDOWS;
                pl_min_ws = keys[l_index].ws_c;
            }

            else {
                // Check for value error
                if(!keys[l_index].key_val_c) {
                    BERR (
                        "link key with no value",
                        keys[l_index].key_name
                    );

                    exit(ERRC_BUILD_CFG);
                }

                sauPushToStrArr (
                    &p_bi->cpy_info.all.dsts,
                    &p_bi->cpy_info.all.cpy_c,
                    keys[l_index].key_name
                );

                p_bi->cpy_info.all.cpy_c--;
                sauPushToStrArr (
                    &p_bi->cpy_info.all.srcs,
                    &p_bi->cpy_info.all.cpy_c,
                    *keys[l_index].key_vals
                );
            }
        }

        else if
        (
            is_platform_val &&
            keys[l_index].ws_c > pl_min_ws
        ) {
            // Check for value error
            if(!keys[l_index].key_val_c) {
                BERR (
                    "copy key with no value",
                    keys[l_index].key_name
                );

                exit(ERRC_BUILD_CFG);
            }

            switch (pi)
            {
            case PLATFORM_APPLE:
                sauPushToStrArr (
                    &p_bi->cpy_info.apple_i.dsts,
                    &p_bi->cpy_info.apple_i.cpy_c,
                    keys[l_index].key_name
                );

                p_bi->cpy_info.apple_i.cpy_c--;
                sauPushToStrArr (
                    &p_bi->cpy_info.apple_i.srcs,
                    &p_bi->cpy_info.apple_i.cpy_c,
                    *keys[l_index].key_vals
                );
                break;

            case PLATFORM_LINUX:
                sauPushToStrArr (
                    &p_bi->cpy_info.linux_i.dsts,
                    &p_bi->cpy_info.linux_i.cpy_c,
                    keys[l_index].key_name
                );

                p_bi->cpy_info.linux_i.cpy_c--;
                
                sauPushToStrArr (
                    &p_bi->cpy_info.linux_i.srcs,
                    &p_bi->cpy_info.linux_i.cpy_c,
                    *keys[l_index].key_vals
                );
                break;

            case PLATFORM_WINDOWS:
                sauPushToStrArr (
                    &p_bi->cpy_info.win_i.dsts,
                    &p_bi->cpy_info.win_i.cpy_c,
                    keys[l_index].key_name
                );

                p_bi->cpy_info.win_i.cpy_c--;
                sauPushToStrArr (
                    &p_bi->cpy_info.win_i.srcs,
                    &p_bi->cpy_info.win_i.cpy_c,
                    *keys[l_index].key_vals
                );
                break;
            
            default:
                break;
            }
        }

        else if 
        (
            is_platform_val &&
            keys[l_index].ws_c <= pl_min_ws &&
            keys[l_index].ws_c > base_ws
        )  {
            is_platform_val = false;
            l_index--;
        }

        else if(keys[l_index].ws_c <= base_ws) return;
    }
}


/* Create build info needed for makefile creation */
void sauAssembleBuildData (
    KeyData *keys, 
    int32_t key_c, 
    BuildInfo *p_bi
) {
    int32_t l_index;
    // Find the base whitespace count
    for(l_index = 0; l_index < key_c; l_index++) {
        if(!l_index) base_ws = keys[l_index].ws_c;

        if(!keys[l_index].ws_c) {
            base_ws = keys[l_index].ws_c;
            break;
        }
        else if(keys[l_index].ws_c < base_ws) 
            base_ws = keys[l_index].ws_c;
    }

    sauInitPremakeValues(p_bi);
    sauAssemblePremake(keys, key_c, p_bi);
    sauAssembleTasks(keys, key_c, p_bi);
    sauFindLinks(keys, key_c, p_bi);
    sauFindCopies(keys, key_c, p_bi);
}