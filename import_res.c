#define IR_PRIVATE 1
#include "saucer.h"

/* Check if path is absolute */
static uint8_t isPathAbsolute(char *path) {
    uint8_t is_absolute = false;
    // Unix like path check
    if(path[0] == 0x2F)
        is_absolute = true;

    // Windows drive letter check
    else if
    (
        ((path[0] < MAX_UPPER_CASE && path[0] >= MIN_UPPER_CASE) ||
        (path[0] < MAX_LOWER_CASE && path[0] >= MIN_LOWER_CASE)) &&
        path[1] == 0x3A
    ) is_absolute = true;

    return is_absolute;
}


/* Find all imports inside build info */
static void irFindImports (
    BuildInfo *p_bi,
    char ***p_imports,
    int32_t *p_import_c
) {
    (*p_imports) = NULL;
    (*p_import_c) = 0;
    
    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        cmbStrArr (
            p_bi->import.all.imports,
            p_bi->import.all.import_c,
            p_bi->import.apple_i.imports,
            p_bi->import.apple_i.import_c,
            p_imports,
            p_import_c
        );
        break;

    case PLATFORM_LINUX:
        cmbStrArr (
            p_bi->import.all.imports,
            p_bi->import.all.import_c,
            p_bi->import.linux_i.imports,
            p_bi->import.linux_i.import_c,
            p_imports,
            p_import_c
        );
        break;

    case PLATFORM_WINDOWS:
        cmbStrArr (
            p_bi->import.all.imports,
            p_bi->import.all.import_c,
            p_bi->import.win_i.imports,
            p_bi->import.win_i.import_c,
            p_imports,
            p_import_c
        );
        break;
    
    default:
        break;
    }
}


/* Recursive function to create imported build_infos for all imported configs */ 
static void irAssembleImportBuildInfos (
    BuildInfo **p_out_bi,
    int32_t *p_out_c,
    int32_t *p_cur_index,
    char **import_paths,
    int32_t import_c,
    char *prev_im_path,
    uint8_t is_recursion
) {
    int32_t l_index;

    char *cfg_path = NULL;
    int32_t cfg_path_c = 0;
    
    KeyData *keys;
    int32_t key_c;
    
    // Variables for newly found imports
    char **new_imports = NULL;
    int32_t new_im_c = 0;

    // Variables for newly found import paths relative to original config location
    char **rel_im_paths = NULL;
    int32_t rel_im_path_c = 0;
    
    // Check if import paths need to be made relative
    if
    (
        is_recursion && 
        prev_im_path
    ) {
        // Find the previous config file path
        for(cfg_path_c = strlen(prev_im_path) - 1; cfg_path_c >= 0; cfg_path_c--)
            if(prev_im_path[cfg_path_c] == 0x2F) break;

        // Check if import paths need to be changed according to previous path
        for(l_index = 0; l_index < import_c; l_index++) {
            if(!isPathAbsolute(import_paths[l_index])) {
                char *tmp = strnPathMerge (
                    prev_im_path,
                    cfg_path_c,
                    import_paths[l_index],
                    strlen(import_paths[l_index])
                );

                free(import_paths[l_index]);
                import_paths[l_index] = tmp;
            }
        }
    }
    
    // Allocate more memory for buildinfos if needed
    if(is_recursion) {
        BuildInfo *tmp_bi = (BuildInfo*) realloc (
            (*p_out_bi),
            (*p_out_c) * sizeof(BuildInfo)
        );

        if(!tmp_bi) {
            MEMERR("Failed to reallocate memory for imported buildinfos");
            exit(ERRC_MEM);
        }

        (*p_out_bi) = tmp_bi;
    }
    
    for(l_index = 0; (*p_cur_index) < (*p_out_c); (*p_cur_index)++, l_index++) {
        yamlParse(&keys, &key_c, import_paths[l_index]);
        sauAssembleBuildData (
            keys, 
            key_c, 
            &(*p_out_bi)[(*p_cur_index)], 
            true
        );
        
        // Find source path relative to main build config
        if(!isPathAbsolute((*p_out_bi)[(*p_cur_index)].premake.src_dir)) {
            if(prev_im_path) {
                for(cfg_path_c = strlen(prev_im_path); cfg_path_c >= 0; cfg_path_c--)
                    if(prev_im_path[cfg_path_c] == 0x2F) break;
                cfg_path_c++;

                // Merge cfg path with previous path
                cfg_path = strnPathMerge (
                    prev_im_path,
                    cfg_path_c,
                    import_paths[l_index],
                    strlen(import_paths[l_index])
                );

                free(prev_im_path);

                // Find build config file path max index
                for(cfg_path_c = strlen(cfg_path) - 1; cfg_path_c >= 0; cfg_path_c--)
                    if(cfg_path[cfg_path_c] == 0x2F) break;

                cfg_path_c++;
            }

            else {
                // Find directory path bound
                for(cfg_path_c = strlen(import_paths[l_index]) - 1; cfg_path_c >= 0; cfg_path_c--)
                    if(import_paths[l_index][cfg_path_c] == 0x2F) break;
                cfg_path_c++;

                cfg_path = (char*) calloc (
                    cfg_path_c + 1,
                    sizeof(char)
                );

                strncpy (
                    cfg_path,
                    import_paths[l_index],
                    cfg_path_c    
                );

                // Find full previous path directory path bound
                for(cfg_path_c = strlen(cfg_path) - 1; cfg_path_c >= 0; cfg_path_c--)
                    if(cfg_path[cfg_path_c] == 0x2F) break;

                cfg_path_c++;
            }
            
            // Merge paths to get path relative to initial config file location
            char *tmp_str = strnPathMerge (
                cfg_path,
                cfg_path_c,
                (*p_out_bi)[(*p_cur_index)].premake.src_dir,
                strlen((*p_out_bi)[(*p_cur_index)].premake.src_dir) 
            );

            // Assign newly formed source directory variable as src_dir
            free((*p_out_bi)[(*p_cur_index)].premake.src_dir);
            (*p_out_bi)[(*p_cur_index)].premake.src_dir = tmp_str;
            free(cfg_path);
        }

        switch ((*p_out_bi)[(*p_cur_index)].platform) 
        {
        case PLATFORM_APPLE:
            cmbStrArr (
                (*p_out_bi)[(*p_cur_index)].import.all.imports,
                (*p_out_bi)[(*p_cur_index)].import.all.import_c,
                (*p_out_bi)[(*p_cur_index)].import.apple_i.imports,
                (*p_out_bi)[(*p_cur_index)].import.apple_i.import_c,
                &new_imports,
                &new_im_c
            );
            break;

        case PLATFORM_LINUX:
            cmbStrArr (
                (*p_out_bi)[(*p_cur_index)].import.all.imports,
                (*p_out_bi)[(*p_cur_index)].import.all.import_c,
                (*p_out_bi)[(*p_cur_index)].import.linux_i.imports,
                (*p_out_bi)[(*p_cur_index)].import.linux_i.import_c,
                &new_imports,
                &new_im_c
            );
            break;
        
        case PLATFORM_WINDOWS:
            cmbStrArr (
                (*p_out_bi)[(*p_cur_index)].import.all.imports,
                (*p_out_bi)[(*p_cur_index)].import.all.import_c,
                (*p_out_bi)[(*p_cur_index)].import.win_i.imports,
                (*p_out_bi)[(*p_cur_index)].import.win_i.import_c,
                &new_imports,
                &new_im_c
            );
            break;

        default:
            break;
        }

        if(prev_im_path) free(prev_im_path); 
        if(new_im_c) {
            (*p_out_c) += new_im_c;
            (*p_cur_index)++;
            
            irAssembleImportBuildInfos (
                p_out_bi,
                p_out_c,
                p_cur_index,
                new_imports,
                new_im_c,
                cfg_path,
                true
            );
        }
    }

    if((*p_cur_index)) (*p_cur_index)--;
}


/* Main callback function for creating buildinfos from imported build configs */
void irCreateImportBuildInfos (
    BuildInfo *p_main_bi,
    BuildInfo **p_out_bis,
    int32_t *p_out_c
) {
    int32_t l_index; 
    // Initialise empty out task values
    (*p_out_c) = 0;
    (*p_out_bis) = NULL;

    // Find the total count of all imports
    char **imports = NULL;

    switch (p_main_bi->platform)
    {
    case PLATFORM_APPLE: 
        cmbStrArr (
            p_main_bi->import.all.imports,
            p_main_bi->import.all.import_c,
            p_main_bi->import.apple_i.imports,
            p_main_bi->import.apple_i.import_c,
            &imports,
            p_out_c
        );
        break;
    
    case PLATFORM_LINUX:
        cmbStrArr (
            p_main_bi->import.all.imports,
            p_main_bi->import.all.import_c,
            p_main_bi->import.linux_i.imports,
            p_main_bi->import.linux_i.import_c,
            &imports,
            p_out_c
        );
        break; 
    
    case PLATFORM_WINDOWS:
        cmbStrArr (
            p_main_bi->import.all.imports,
            p_main_bi->import.all.import_c,
            p_main_bi->import.win_i.imports,
            p_main_bi->import.win_i.import_c,
            &imports,
            p_out_c
        );
        break;

    default:
        break;
    }

    if(!(*p_out_c)) return;

    // Allocate memory for output buildinfo
    (*p_out_bis) = (BuildInfo*) calloc (
        (*p_out_c),
        sizeof(BuildInfo)
    );

    l_index = 0;
    irAssembleImportBuildInfos (
        p_out_bis,
        p_out_c,
        &l_index,
        imports,
        *p_out_c,
        NULL,
        false
    );
}