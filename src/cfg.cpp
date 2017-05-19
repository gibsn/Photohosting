#include "cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iniparser.h"

#include "log.h"


#define CFG_INIT CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    VAR = CFG_##TYPE##_DEF_VALUE;

Config::Config()
    : path_to_cfg(NULL)
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
    if (path_to_cfg) free(path_to_cfg);

    CFG_FREE
}

#undef CFG_ENTRY
#undef CFG_FREE


#define CFG_PARSE_INI CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
        CFG_##TYPE##_PARSE(dict, VAR, SECTION, NAME);

bool Config::Init(const char *path_to_cfg)
{
    dictionary *dict = iniparser_load(path_to_cfg);
    if (!dict) {
        fprintf(stderr, "Failed to initialise config\n");
        return false;
    }

    CFG_PARSE_INI

    iniparser_freedict(dict);
    return true;
}

#undef CFG_ENTRY
#undef CFG_PARSE_INI


#define CFG_CHECK CFG_GEN
#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    if (VAR == CFG_##TYPE##_DEF_VALUE) {               \
        fprintf(stderr,                                \
            "You have not specified " NAME ", "        \
            "using " #DEF_VALUE " by default\n");      \
        CFG_##TYPE##_ASSIGN(VAR, DEF_VALUE);           \
    }

void Config::Check()
{
    CFG_CHECK
}

#undef CFG_ENTRY
#undef CFG_CHECK
