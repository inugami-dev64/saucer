#ifndef SAUCER_H
#define SAUCER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "yaml_parser.h"
#include "build_maker.h"
#include "script_man.h"
#include "make_writer.h"
#include "str_ext.h"

/* Error exit code macros */
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
#endif

#endif