#include "cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iniparser.h"

#include "log.h"


#define CFG_PATH_TO_CFG_DEFAULT "./photohosting.ini"

#define CFG_INIT CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    VAR = CFG_##TYPE##_DEF_VALUE;

Config::Config()
{
    CFG_INIT
}

#undef CFG_ENTRY
#undef CFG_INIT


#define CFG_FREE CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    CFG_##TYPE##_FREE(VAR)

Config::~Config()
{
    CFG_FREE
}

#undef CFG_ENTRY
#undef CFG_FREE


#define CFG_PARSE_INI CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
        CFG_##TYPE##_PARSE(dict, VAR, SECTION, NAME);

bool Config::Parse(const char *path_to_cfg)
{
    if (!path_to_cfg) {
        fprintf(stderr,
            "You have not specified path to config, "
            "using " CFG_PATH_TO_CFG_DEFAULT " by default\n"
        );

        path_to_cfg = CFG_PATH_TO_CFG_DEFAULT;
    }

    dictionary *dict = iniparser_load(path_to_cfg);
    if (!dict) {
        fprintf(stderr, "Failed to initialise config\n");
        return false;
    }

    CFG_PARSE_INI

    iniparser_freedict(dict);

    PrintDefaults();

    return true;
}

#undef CFG_ENTRY
#undef CFG_PARSE_INI


#define CFG_PRINT_DEFAULTS CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    if (VAR == CFG_##TYPE##_DEF_VALUE) {               \
        fprintf(stderr,                                \
            "You have not specified " NAME ", "        \
            "using " #DEF_VALUE " by default\n");      \
        CFG_##TYPE##_ASSIGN(VAR, DEF_VALUE);           \
    }

void Config::PrintDefaults()
{
    CFG_PRINT_DEFAULTS
}

#undef CFG_ENTRY
#undef CFG_CHECK
