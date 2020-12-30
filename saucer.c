#define SAUCER_PRIVATE
#include "saucer.h"

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
        remove("Makefile");
        remove("init.sh");
        remove("init.bat");
        printf("Done!\n");
        exit(0);
    }

    yamlParse(&keys, &key_c, argv[1]);
    BuildInfo bi;
    sauAssembleBuildData(keys, key_c, &bi);
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

    sauWriteMakefile(&bi);

    // buildLog(&bi);

    switch (bi.platform)
    {
    case PLATFORM_APPLE:
        printf("Successfully generated makefile with MacOS configuration!\n");
        break;

    case PLATFORM_LINUX:
        printf("Successfully generated makefile with Linux configuration!\n");
        break;

    case PLATFORM_WINDOWS:
        printf("Successfully generated makefile with Windows configuration!\n");
        break;
    
    default:
        break;
    }

    return 0;
}