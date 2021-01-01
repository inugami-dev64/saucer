#ifndef IMPORT_RES_H
#define IMPORT_RES_H


#ifdef IR_PRIVATE

    /* Check if path is absolute */
    static uint8_t isPathAbsolute(char *path);
    static void irAssembleImportBuildInfos (
        BuildInfo **p_out_bi, 
        int32_t *p_out_c, 
        int32_t *p_cur_index,
        char **import_paths,
        int32_t import_c,
        char *prev_im_path,
        uint8_t is_recursion
    );

    static void irFindImports (
        BuildInfo *p_bi,
        char ***p_imports,
        int32_t *p_import_c
    );
#endif

// Create new buildinfos for imported tasks
void irCreateImportBuildInfos(BuildInfo *p_main_bi, BuildInfo **p_out_bi, int32_t *p_out_c);

#endif