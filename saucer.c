#define SAUCER_PRIVATE
#include "saucer.h"

/* Log information about build to console */
static void buildLog(BuildInfo *p_bi) {
    int32_t l_index, r_index;
    // LOG project info
    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        printf("PLATFORM: MacOS\n");
        break;

    case PLATFORM_LINUX:
        printf("PLATFORM: Linux\n");
        break;
    
    case PLATFORM_WINDOWS:
        printf("PLATFORM: Windows\n");
        break;
    
    default:
        break;
    }
    if(p_bi->premake.cc_info.cc)
        printf("CC: %s\n", p_bi->premake.cc_info.cc);
    if(p_bi->premake.cc_info.cxx)
        printf("CXX: %s\n", p_bi->premake.cc_info.cxx);
    
    printf("PROJECT: %s\n", p_bi->premake.project_name);
    printf("SRC_DIR: %s\n", p_bi->premake.src_dir);
    printf("BUILD_DIR: %s\n", p_bi->premake.build_dir);

    printf("INCLUDE_PATH: \n");
    for(l_index = 0; l_index < p_bi->premake.incl_info.all.path_c; l_index++)
        printf("--%s\n", p_bi->premake.incl_info.all.paths[l_index]);

    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        for(l_index = 0; l_index < p_bi->premake.incl_info.apple_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.incl_info.apple_i.paths[l_index]);
        break;

    case PLATFORM_LINUX:
        for(l_index = 0; l_index < p_bi->premake.incl_info.linux_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.incl_info.linux_i.paths[l_index]);
        break;

    case PLATFORM_WINDOWS:
        for(l_index = 0; l_index < p_bi->premake.incl_info.win_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.incl_info.win_i.paths[l_index]);
        break;
    
    default:
        break;
    }

    printf("LIB_PATH: \n");
    for(l_index = 0; l_index < p_bi->premake.incl_info.all.path_c; l_index++)
        printf("--%s\n", p_bi->premake.incl_info.all.paths[l_index]);

    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        for(l_index = 0; l_index < p_bi->premake.lib_info.apple_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.lib_info.apple_i.paths[l_index]);
        break;

    case PLATFORM_LINUX:
        for(l_index = 0; l_index < p_bi->premake.lib_info.linux_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.lib_info.linux_i.paths[l_index]);
        break;

    case PLATFORM_WINDOWS:
        for(l_index = 0; l_index < p_bi->premake.lib_info.win_i.path_c; l_index++)
            printf("--%s\n", p_bi->premake.lib_info.win_i.paths[l_index]);
        break;
    
    default:
        break;
    }

    for(l_index = 0; l_index < p_bi->task_c; l_index++) {
        printf("\nTASK:\n");
        printf("NAME: %s\n", p_bi->tasks[l_index].name);
        
        switch (p_bi->tasks[l_index].type)
        {
        case TARGET_TYPE_DYNAMIC_LIBRARY:
            printf("TYPE: DYNAMIC LIBRARY\n");
            break;

        case TARGET_TYPE_STATIC_LIBRARY:
            printf("TYPE: STATIC LIBRARY\n");
            break;

        case TARGET_TYPE_EXECUTABLE:
            printf("TYPE: EXECUTABLE\n");
            break;
        
        default:
            break;
        }
        
        printf("SRC:\n");
        for(r_index = 0; r_index < p_bi->tasks[l_index].src_info.all.file_c; r_index++)
            printf("--%s\n", p_bi->tasks[l_index].src_info.all.files[r_index]);

        // Platform specific sources
        switch (p_bi->platform)
        {
        case PLATFORM_APPLE:
            for(r_index = 0; r_index < p_bi->tasks[l_index].src_info.apple_i.file_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].src_info.apple_i.files[r_index]);
            break;

        case PLATFORM_LINUX:
            for(r_index = 0; r_index < p_bi->tasks[l_index].src_info.linux_i.file_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].src_info.linux_i.files[r_index]);
            break;

        case PLATFORM_WINDOWS:
            printf("Windows size: %d\n", p_bi->tasks[l_index].src_info.win_i.file_c);
            for(r_index = 0; r_index < p_bi->tasks[l_index].src_info.win_i.file_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].src_info.win_i.files[r_index]);
            break;
        
        default:
            break;
        }

        printf("DEPS:\n");
        for(r_index = 0; r_index < p_bi->tasks[l_index].deps_info.all.deps_c; r_index++)
            printf("--%s\n", p_bi->tasks[l_index].deps_info.all.deps[r_index]);

        // Platform specific sources
        switch (p_bi->platform)
        {
        case PLATFORM_APPLE:
            for(r_index = 0; r_index < p_bi->tasks[l_index].deps_info.apple_i.deps_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].deps_info.apple_i.deps[r_index]);
            break;

        case PLATFORM_LINUX:
            for(r_index = 0; r_index < p_bi->tasks[l_index].deps_info.linux_i.deps_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].deps_info.linux_i.deps[r_index]);
            break;

        case PLATFORM_WINDOWS:
            for(r_index = 0; r_index < p_bi->tasks[l_index].deps_info.win_i.deps_c; r_index++)
                printf("--%s\n", p_bi->tasks[l_index].deps_info.win_i.deps[r_index]);
            break;
        
        default:
            break;
        }
    }

    printf("LINKS:\n");
    for(l_index = 0; l_index < p_bi->link_info.all.link_c; l_index++)
        printf (
            "--%s from %s\n", 
            p_bi->link_info.all.links[l_index],
            p_bi->link_info.all.srcs[l_index]
        );

    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        for(l_index = 0; l_index < p_bi->link_info.apple_i.link_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->link_info.apple_i.links[l_index],
                p_bi->link_info.apple_i.srcs[l_index]
            );
        break;

    case PLATFORM_LINUX:
        for(l_index = 0; l_index < p_bi->link_info.linux_i.link_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->link_info.linux_i.links[l_index],
                p_bi->link_info.linux_i.srcs[l_index]
            );
        break;
    
    case PLATFORM_WINDOWS:
        for(l_index = 0; l_index < p_bi->link_info.win_i.link_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->link_info.win_i.links[l_index],
                p_bi->link_info.win_i.srcs[l_index]
            );
        break;
    
    default:
        break;
    }

    
    printf("COPIES:\n");
    for(l_index = 0; l_index < p_bi->cpy_info.all.cpy_c; l_index++)
        printf (
            "--%s from %s\n", 
            p_bi->cpy_info.all.dsts[l_index],
            p_bi->cpy_info.all.srcs[l_index]
        );

    switch (p_bi->platform)
    {
    case PLATFORM_APPLE:
        for(l_index = 0; l_index < p_bi->cpy_info.apple_i.cpy_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->cpy_info.apple_i.dsts[l_index],
                p_bi->cpy_info.apple_i.srcs[l_index]
            );
        break;

    case PLATFORM_LINUX:
        for(l_index = 0; l_index < p_bi->cpy_info.linux_i.cpy_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->cpy_info.linux_i.dsts[l_index],
                p_bi->cpy_info.linux_i.srcs[l_index]
            );
        break;
    
    case PLATFORM_WINDOWS:
        for(l_index = 0; l_index < p_bi->cpy_info.win_i.cpy_c; l_index++)
            printf (
                "--%s from %s\n", 
                p_bi->cpy_info.win_i.dsts[l_index],
                p_bi->cpy_info.win_i.srcs[l_index]
            );
        break;
    
    default:
        break;
    }
}


/* Clean makefile and all initialisation scripts */
static void cleanMake (
    uint8_t *p_mk_fb, 
    uint8_t *p_init_bat_fb, 
    uint8_t *p_init_sh_fb
) {
    DIR *dir;
    struct dirent *contents;
    dir = opendir(".");
    while((contents = readdir(dir))) {
        if
        (
            contents->d_type == DT_REG &&
            !strcmp(contents->d_name, "Makefile")
        ) {
            (*p_mk_fb) = true;
            remove(contents->d_name);
        }

        else if
        (
            contents->d_type == DT_REG &&
            !strcmp (
                contents->d_name, 
                SCRIPT_NAME \
                ".sh"
            )
        ) {
            (*p_init_sh_fb) = true;
            remove(contents->d_name);
        }

        else if
        (
            contents->d_type == DT_REG &&
            !strcmp (
                contents->d_name, 
                SCRIPT_NAME \
                ".bat"
            )
        ) {
            (*p_init_bat_fb) = true;
            remove(contents->d_name);
        }
    }

    closedir(dir);
}


/* Clean all heap allocated build information */ 
static void cleanBuild(BuildInfo *p_bi) {
    // Free project data
    free(p_bi->premake.src_dir);
    free(p_bi->premake.build_dir);
    free(p_bi->premake.project_name);
    free(p_bi->all_task);
    free(p_bi->premake.cc_info.cc);
    free(p_bi->premake.cc_info.cxx);

    // Free tasks information
    int32_t l_index;
    for(l_index = 0; l_index < p_bi->task_c; l_index++) {
        free(p_bi->tasks[l_index].name);
        // Task sources and objects
        if(p_bi->tasks[l_index].src_info.all.files) {
            free(p_bi->tasks[l_index].src_info.all.files);
            free(p_bi->tasks[l_index].obj_info.all.files);
        }
        if(p_bi->tasks[l_index].src_info.apple_i.files) {
            free(p_bi->tasks[l_index].src_info.apple_i.files);
            free(p_bi->tasks[l_index].obj_info.apple_i.files);
        }
        if(p_bi->tasks[l_index].src_info.linux_i.files) {
            free(p_bi->tasks[l_index].src_info.linux_i.files);
            free(p_bi->tasks[l_index].obj_info.linux_i.files);
        }
        if(p_bi->tasks[l_index].src_info.win_i.files) {
            free(p_bi->tasks[l_index].src_info.win_i.files);
            free(p_bi->tasks[l_index].obj_info.win_i.files);
        }

        // Task dependencies
        if(p_bi->tasks[l_index].deps_info.all.deps)
            free(p_bi->tasks[l_index].deps_info.all.deps);
        if(p_bi->tasks[l_index].deps_info.all.deps)
            free(p_bi->tasks[l_index].deps_info.apple_i.deps);
        if(p_bi->tasks[l_index].deps_info.all.deps)
            free(p_bi->tasks[l_index].deps_info.linux_i.deps);
        if(p_bi->tasks[l_index].deps_info.all.deps)
            free(p_bi->tasks[l_index].deps_info.win_i.deps);
    }

    free(p_bi->tasks);

    // Free all cc flags data in a build
    if(p_bi->premake.cc_flags.all.flags)
        free(p_bi->premake.cc_flags.all.flags);
    if(p_bi->premake.cc_flags.apple_i.flags)
        free(p_bi->premake.cc_flags.apple_i.flags);
    if(p_bi->premake.cc_flags.linux_i.flags)
        free(p_bi->premake.cc_flags.linux_i.flags);
    if(p_bi->premake.cc_flags.win_i.flags)
        free(p_bi->premake.cc_flags.win_i.flags);

    // Free all cxx flags data in a build
    if(p_bi->premake.cxx_flags.all.flags)
        free(p_bi->premake.cxx_flags.all.flags);
    if(p_bi->premake.cxx_flags.apple_i.flags)
        free(p_bi->premake.cxx_flags.apple_i.flags);
    if(p_bi->premake.cxx_flags.linux_i.flags)
        free(p_bi->premake.cxx_flags.linux_i.flags);
    if(p_bi->premake.cxx_flags.win_i.flags)
        free(p_bi->premake.cxx_flags.win_i.flags);

    // Free all include flags data in a build
    if(p_bi->premake.incl_info.all.paths)
        free(p_bi->premake.incl_info.all.paths);
    if(p_bi->premake.incl_info.apple_i.paths)
        free(p_bi->premake.incl_info.apple_i.paths);
    if(p_bi->premake.incl_info.linux_i.paths)
        free(p_bi->premake.incl_info.linux_i.paths);
    if(p_bi->premake.incl_info.win_i.paths)
        free(p_bi->premake.incl_info.win_i.paths);

    // Free all library flags data in a build
    if(p_bi->premake.lib_info.all.paths)
        free(p_bi->premake.lib_info.all.paths);
    if(p_bi->premake.lib_info.apple_i.paths)
        free(p_bi->premake.lib_info.apple_i.paths);
    if(p_bi->premake.lib_info.linux_i.paths)
        free(p_bi->premake.lib_info.linux_i.paths);
    if(p_bi->premake.lib_info.win_i.paths)
        free(p_bi->premake.lib_info.win_i.paths);


    // Free all copy datas
    free(p_bi->cpy_info.all.dsts);
    free(p_bi->cpy_info.all.srcs);
    free(p_bi->cpy_info.apple_i.dsts);
    free(p_bi->cpy_info.apple_i.srcs);
    free(p_bi->cpy_info.linux_i.dsts);
    free(p_bi->cpy_info.linux_i.srcs);
    free(p_bi->cpy_info.win_i.dsts);
    free(p_bi->cpy_info.win_i.srcs);

    // Free all link datas
    free(p_bi->link_info.all.links);
    free(p_bi->link_info.all.srcs);
    free(p_bi->link_info.apple_i.links);
    free(p_bi->link_info.apple_i.srcs); 
    free(p_bi->link_info.linux_i.links);
    free(p_bi->link_info.linux_i.srcs);
    free(p_bi->link_info.win_i.links);
    free(p_bi->link_info.win_i.srcs);
} 


int main(int argc, char *argv[]) {
    KeyData *keys;
    int32_t key_c;
    // Check for usage error
    if(argc == 1) {
        USEERR("no config file specified");
        exit(ERRC_USE);
    }

    if(!strcmp(argv[1], "--help")) {
        printf("%s", help_message);
        exit(0);
    }

    else if(!strcmp(argv[1], "clean")) {
        char *tmp_str = (char*) calloc (
            YAML_CH_BUFFER_SIZE,
            sizeof(char)
        );

        uint8_t rm_mk = false;
        uint8_t rm_init_sh = false;
        uint8_t rm_init_bat = false;
        cleanMake(&rm_mk, &rm_init_bat, &rm_init_sh);

        if
        (
            !rm_mk && 
            !rm_init_bat &&
            !rm_init_sh
        ) {
            printf("Nothing to clean here!\n");
            exit(ERRC_NONE);
        }

        int32_t offset = 0;
        sprintf (
            tmp_str + offset,
            "Removed: "
        );

        offset += 9;

        if(rm_mk) {
            sprintf (
                tmp_str + offset,
                "Makefile"
            );

            offset += 8;
        }

        if(rm_init_sh) {
            sprintf (
                tmp_str + offset,
                ", %s.sh",
                SCRIPT_NAME
            );

            offset += strlen(SCRIPT_NAME) + 5;
        }

        if(rm_init_bat) {
            sprintf (
                tmp_str + offset,
                ", %s.bat",
                SCRIPT_NAME
            );

            offset += strlen(SCRIPT_NAME) + 6;
        }

        printf("%s\n", tmp_str);
        free(tmp_str);
        exit(ERRC_NONE);
    }

    yamlParse(&keys, &key_c, argv[1]);
    BuildInfo bi;
    sauAssembleBuildData(keys, key_c, &bi, false);
    // buildLog(&bi);

    // Check for platform specification
    if
    (
        argc == 4 &&
        !strcmp(argv[2], "-p") &&
        !strcmp(argv[3], "apple")
    ) bi.platform = PLATFORM_APPLE;

    else if
    (
        argc == 4 &&
        !strcmp(argv[2], "-p") &&
        !strcmp(argv[3], "linux")
    ) bi.platform = PLATFORM_LINUX;

    else if
    (
        argc == 4 &&
        !strcmp(argv[2], "-p") &&
        !strcmp(argv[3], "windows")
    ) bi.platform = PLATFORM_WINDOWS;

    BuildInfo *imports;
    int32_t im_c;

    irCreateImportBuildInfos(&bi, &imports, &im_c);
    sauInitMakefileVars(&bi, imports, im_c);
    sauWriteMakefile(&bi, imports, im_c);

    // buildLog(&bi);

    switch (bi.platform)
    {
    case PLATFORM_APPLE:
        printf("Generated a Makefile with MacOS configuration!\n");
        break;

    case PLATFORM_LINUX:
        printf("Generated a Makefile with Linux configuration!\n");
        break;

    case PLATFORM_WINDOWS:
        printf("Generated a Makefile with Windows configuration!\n");
        break;
    
    default:
        break;
    }

    // Output information about tasks
    printf("Master build tasks: %d\n", bi.task_c);
    printf("Imported builds: %d\n", im_c);

    cleanBuild(&bi);
    for(int32_t index = 0; index < im_c; index++) 
        cleanBuild(&imports[index]);
    return 0;
}