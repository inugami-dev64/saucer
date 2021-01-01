#define SCRIPT_MAN_PRIVATE
#include "saucer.h"

/* Find all directories needed for build */
static void smFindDirs (
    TaskInfo *tasks, 
    int32_t task_c, 
    char *pr_name, 
    char ***p_paths, 
    int32_t *p_path_c,
    uint8_t is_win,
    uint8_t is_import
) {
    int32_t l_index, r_index;
    uint8_t is_lib_tar = false;

    if(!is_import) {
        (*p_path_c) += 2 + task_c;

        // Find if library task exists
        for(l_index = 0; l_index < task_c; l_index++) {
            if(tasks[l_index].type != TARGET_TYPE_EXECUTABLE) {
                (*p_path_c)++;
                is_lib_tar = true;
                break;
            } 
        }

        // Allocate memory for paths
        char **tmp = (char**) realloc (
            (*p_paths),
            (*p_path_c) * sizeof(char*)
        );

        // Check for memory allocation error
        if(!tmp) {
            MEMERR("failed to reallocate memory for new path names");
            exit(ERRC_MEM);
        }

        (*p_paths) = tmp;
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
    }

    else {
        l_index = (*p_path_c);
        (*p_path_c) += task_c;
        
        // Allocate memory for paths
        char **tmp = (char**) realloc (
            (*p_paths),
            (*p_path_c) * sizeof(char*)
        );

        if(!tmp) {
            MEMERR("failed to reallocate memory for new path names");
            exit(ERRC_MEM);
        }

        (*p_paths) = tmp;
    } 

    // Find the rest of the sub directory names in obj/ folder 
    for(r_index = 0; r_index < task_c; l_index++, r_index++) {
        (*p_paths)[l_index] = (char*) calloc (
            strlen(tasks[r_index].name) + 5,
            sizeof(char)
        );

        if(is_win) {
            sprintf (
                (*p_paths)[l_index],
                "obj\\%s",
                tasks[r_index].name
            );
        }
        else {
            sprintf (
                (*p_paths)[l_index],
                "obj/%s",
                tasks[r_index].name
            );
        }
    }
}


/* Find all symlinks and their sources */
static void smFindLinks (
    BuildInfo *p_bi,
    char ***p_links,
    char ***p_srcs,
    int32_t *p_link_c
) {
    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        cmbStrArr (
            p_bi->link_info.all.links,
            p_bi->link_info.all.link_c,
            p_bi->link_info.apple_i.links,
            p_bi->link_info.apple_i.link_c,
            p_links,
            p_link_c
        );

        cmbStrArr (
            p_bi->link_info.all.srcs,
            p_bi->link_info.all.link_c,
            p_bi->link_info.apple_i.srcs,
            p_bi->link_info.apple_i.link_c,
            p_srcs,
            p_link_c
        );
        break;

    case PLATFORM_LINUX:
        cmbStrArr (
            p_bi->link_info.all.links,
            p_bi->link_info.all.link_c,
            p_bi->link_info.linux_i.links,
            p_bi->link_info.linux_i.link_c,
            p_links,
            p_link_c
        );

        cmbStrArr (
            p_bi->link_info.all.srcs,
            p_bi->link_info.all.link_c,
            p_bi->link_info.linux_i.srcs,
            p_bi->link_info.linux_i.link_c,
            p_srcs,
            p_link_c
        );
        break;

    case PLATFORM_WINDOWS:
        cmbStrArr (
            p_bi->link_info.all.links,
            p_bi->link_info.all.link_c,
            p_bi->link_info.win_i.links,
            p_bi->link_info.win_i.link_c,
            p_links,
            p_link_c
        );

        cmbStrArr (
            p_bi->link_info.all.srcs,
            p_bi->link_info.all.link_c,
            p_bi->link_info.win_i.srcs,
            p_bi->link_info.win_i.link_c,
            p_srcs,
            p_link_c
        );
        break;
    
    default:
        break;
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
    BuildInfo *p_bi,
    BuildInfo *imports,
    int32_t im_c
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

    char **paths = (char**) calloc (
        1,
        sizeof(char)
    );
    int32_t path_c = 0;
    
    // Find base directories needed to create makefile
    smFindDirs (
        p_bi->tasks,
        p_bi->task_c,
        p_bi->premake.project_name,
        &paths,
        &path_c,
        false,
        false
    );

    // Find all import task build folders
    for(l_index = 0; l_index < im_c; l_index++) {
        // Find all directories that 
        smFindDirs (
            imports[l_index].tasks,
            imports[l_index].task_c,
            imports[l_index].premake.project_name,
            &paths,
            &path_c,
            false,
            true
        );
    }

    // Declare shebang
    switch (p_bi->platform)
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


    // Find all links for specified platform
    char **links = NULL;
    char **srcs = NULL;
    int32_t link_c = 0;
    smFindLinks (
        p_bi,
        &links,
        &srcs,
        &link_c
    );

    // Write all linking commands
    for(l_index = 0; l_index < link_c; l_index++) {
        tmp_str = (char*) calloc (
            (2 * strlen(p_bi->premake.project_name)) + 
            (2 * strlen(links[l_index]))+ strlen(srcs[l_index]) + 66,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "if [ ! -d $1/%s/%s ]\n" \
            "then\n" \
            "    ln -s $(realpath %s) $(realpath $1/%s/%s)\n" \
            "fi\n\n",
            p_bi->premake.project_name,
            links[l_index],
            srcs[l_index],
            p_bi->premake.project_name,
            links[l_index]
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
    BuildInfo *p_bi,
    BuildInfo *imports,
    int32_t im_c
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
    char **paths = (char**) calloc (
        1,
        sizeof(char*)
    );
    int32_t path_c = 0;

    // Find the primary paths needed
    smFindDirs (
        p_bi->tasks,
        p_bi->task_c,
        p_bi->premake.project_name,
        &paths,
        &path_c,
        true,
        false
    );

    for(l_index = 0; l_index < im_c; l_index++) {
        smFindDirs (
            imports[l_index].tasks,
            imports[l_index].task_c,
            imports[l_index].premake.project_name,
            &paths,
            &path_c,
            true,
            true
        );
    }

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
    
    // Find all links for specified platform
    char **links = NULL;
    char **srcs = NULL;
    int32_t link_c = 0;
    smFindLinks (
        p_bi,
        &links,
        &srcs,
        &link_c
    );

    sauFixWindowsPaths (
        links,
        link_c
    );

    sauFixWindowsPaths (
        srcs,
        link_c
    );

    // Write all linking commands
    for(l_index = 0; l_index < link_c; l_index++) {
        tmp_str = (char*) calloc (
            (2 * strlen(p_bi->premake.project_name)) + 
            (2 * strlen(links[l_index]))+ strlen(srcs[l_index]) + 34,
            sizeof(char)
        );

        sprintf (
            tmp_str,
            "if not exist %%1\\%s\\%s (mklink %%1\\%s\\%s %s)\n",
            p_bi->premake.project_name,
            links[l_index],
            p_bi->premake.project_name,
            links[l_index],
            srcs[l_index]
        );

        fwrite (
            tmp_str,
            sizeof(char),
            strlen(tmp_str),
            file
        );

        free(tmp_str);
    }

    fclose(file);
}