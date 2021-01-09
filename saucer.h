#ifndef SAUCER_H
#define SAUCER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define __USE_MISC
#include <dirent.h>

#include "yaml_parser.h"
#include "build_maker.h"
#include "script_man.h"
#include "make_writer.h"
#include "import_res.h"
#include "str_ext.h"

/* Error exit code definitions */
#define ERRC_NONE        0
#define ERRC_USE        -1
#define ERRC_PARSE      -2
#define ERRC_FILE       -3
#define ERRC_MEM        -4
#define ERRC_BUILD_CFG  -5

#define MEMERR(x) printf("Memory allocation error: %s\n", x)
#define USEERR(x) printf("Saucer usage error: %s\n", x)

#ifdef SAUCER_PRIVATE
    char *help_message = "General usage of saucer: \n" \
                         "saucer [BUILD_CONF] \n\n" \
                         "For platform specific makefile configuration use: \n" \
                         "saucer [BUILD_CONF] -p [PLATFORM] \n\n" \
                         "[PLATFORM] can be 'apple', 'linux' or 'windows'\n";

    static void buildLog(BuildInfo *p_pi);
    static void cleanMake(uint8_t *p_mk_fb, uint8_t *p_init_bat_fb, uint8_t *p_init_sh_fb);
    static void cleanBuild(BuildInfo *p_bi);
#endif

#endif