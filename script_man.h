#ifndef SCRIPT_MAN_H
#define SCRIPT_MAN_H

#define APPLE_DEF_SHELL     "#!/bin/zsh\n"
#define LINUX_DEF_SHELL     "#!/bin/bash\n"
#define SCRIPT_NAME         "init"

#define OBJ_I   0
#define MAIN_I  1
#define LIB_I   2

#ifdef SCRIPT_MAN_PRIVATE
    // Directory finders
    static void smFindDirs (
        TaskInfo *tasks, 
        int32_t task_c, 
        char *pr_name, 
        char ***p_paths, 
        int32_t *p_path_c, 
        uint8_t is_win,
        uint8_t is_import
    );
    static void smCleanDirInfo(char **paths, int32_t path_c);
    static void smFindLinks(BuildInfo *p_bi, char ***p_links, char ***p_srcs, int32_t *p_link_c);
#endif

// Main script writer functions
void smWriteBashInit(BuildInfo *p_bi, BuildInfo *imports, int32_t im_c);
void smWriteBatchInit(BuildInfo *p_bi, BuildInfo *imports, int32_t im_c);
#endif