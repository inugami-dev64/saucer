#define SCRIPT_MAN_PRIVATE
#include "saucer.h"

/* Find all directories needed for build */
static void smFindDirs (
    TaskInfo *tasks, 
    int32_t task_c, 
    char *pr_name, 
    char ***p_paths, 
    int32_t *p_path_c,
    uint8_t is_win
) {
    int32_t l_index, r_index;
    uint8_t is_lib_tar = false;

    (*p_path_c) = 2 + task_c;

    // Find if library task exists
    for(l_index = 0; l_index < task_c; l_index++) {
        if(tasks[l_index].type != TARGET_TYPE_EXECUTABLE) {
            (*p_path_c)++;
            is_lib_tar = true;
            break;
        } 
    }

    // Allocate memory for paths
    (*p_paths) = (char**) calloc (
        (*p_path_c),
        sizeof(char*)
    );

    (*p_paths)[OBJ_I] = "obj";
    (*p_paths)[MAIN_I] = pr_name;
    if(is_lib_tar) {
        (*p_paths)[LIB_I] = (char*) calloc (
            strlen((*p_paths)[MAIN_I]) + 5,
            sizeof(char)
        );

        if(is_win) {
            sprintf (
                (*p_paths)[LIB_I],
                "%s\\lib",
                (*p_paths)[MAIN_I]
            );
        }
        else {
            sprintf (
                (*p_paths)[LIB_I],
                "%s/lib",
                (*p_paths)[MAIN_I]
            );
        }
        l_index = LIB_I + 1;
    } 
    else l_index = MAIN_I + 1;

    // Find the rest of the sub directory names in obj/ folder 
    for(r_index = 0; r_index < task_c; l_index++, r_index++) {
        (*p_paths)[l_index] = (char*) calloc (
            strlen((*p_paths)[OBJ_I]) + strlen(tasks[r_index].name) + 2,
            sizeof(char)
        );

        if(is_win) {
            sprintf (
                (*p_paths)[l_index],
                "%s\\%s",
                (*p_paths)[OBJ_I],
                tasks[r_index].name
            );
        }
        else {
            sprintf (
                (*p_paths)[l_index],
                "%s/%s",
                (*p_paths)[OBJ_I],
                tasks[r_index].name
            );
        }
    }
}


/* Cleanup all directory names */
static void smCleanDirInfo (
    char **paths,
    int32_t path_c
) {
    int32_t l_index;
    for(l_index = MAIN_I + 1; l_index < path_c; l_index++)
        free(paths[l_index]);

    free(paths);
}


/* Bash init script writer function */
void smWriteBashInit (
    TaskInfo *tasks,
    int32_t task_c,
    PlatformInfo pi,
    char *project_name
) {
    char *tmp_str;

    FILE *file;
    file = fopen(
        SCRIPT_NAME \
        ".sh", 
        "w"
    );

    if(!file) {
        FERR (
            "failed to open file", 
            SCRIPT_NAME \
            ".sh"
        );
        exit(ERRC_FILE);
    }

    int32_t l_index;

    char **paths;
    int32_t path_c = 0;
    
    // Find all directories needed in build dir
    smFindDirs (
        tasks,
        task_c,
        project_name,
        &paths,
        &path_c,
        false
    );

    // Declare shebang
    switch (pi)
    {
    case PLATFORM_APPLE:
        fwrite (
            APPLE_DEF_SHELL,
            sizeof(char),
            strlen(APPLE_DEF_SHELL),
            file
        );    
        break;

    case PLATFORM_LINUX:
        fwrite (
            LINUX_DEF_SHELL,
            sizeof(char),
            strlen(LINUX_DEF_SHELL),
            file
        );
        break;
    
    default:
        break;
    }

    // Write first if declaration for build directory itself
    tmp_str = (char*) calloc (
        36,
        sizeof(char)
    );

    sprintf (
        tmp_str,
        "if [ ! -d $1 ]\n" \
        "then\n" \
        "    mkdir $1\n" \
        "fi\n\n"
    );

    fwrite (
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );

    free(tmp_str);

    // Write the rest of the strings 
    for(l_index = 0; l_index < path_c; l_index++) {
        tmp_str = (char*) calloc (
            38 + (2 * strlen(paths[l_index])),
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "if [ ! -d $1/%s ]\n" \
            "then\n" \
            "    mkdir $1/%s\n" \
            "fi\n\n",
            paths[l_index],
            paths[l_index]
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    smCleanDirInfo(paths, path_c);
    fclose(file);
}


void smWriteBatchInit (
    TaskInfo *tasks,
    int32_t task_c,
    char *project_name
) {
    char *tmp_str;
    int32_t l_index;

    FILE *file;
    file = fopen ( 
        SCRIPT_NAME \
        ".bat",
        "w"
    );

    if(!file) {
        FERR (
            "failed to open file",
            SCRIPT_NAME \
            ".bat"
        );

        exit(ERRC_FILE);
    }

    // Find all subdirectories
    char **paths;
    int32_t path_c = 0;

    smFindDirs (
        tasks,
        task_c,
        project_name,
        &paths,
        &path_c,
        true
    );

    // Disable console output for this script
    fwrite (
        "@echo off\n\n",
        sizeof(char),
        11,
        file
    );

    // Write first if statement for build directory itself
    tmp_str = (char*) calloc (
        28,
        sizeof(char)
    );

    sprintf (
        tmp_str,
        "if not exist %%1 (mkdir %%1)\n"
    );

    fwrite ( 
        tmp_str,
        sizeof(char),
        strlen(tmp_str),
        file
    );
    
    free(tmp_str);

    // Write the rest of subdirectories
    for(l_index = 0; l_index < path_c; l_index++) {
        tmp_str = (char*) calloc (
            30 + (2 * strlen(paths[l_index])),
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "if not exist %%1\\%s (mkdir %%1\\%s)\n",
            paths[l_index],
            paths[l_index]
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    smCleanDirInfo(paths, path_c);
    fclose(file);
}