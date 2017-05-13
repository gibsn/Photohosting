#ifndef CFG_H_SENTRY
#define CFG_H_SENTRY

#define CFG_INT_TYPE int
#define CFG_INT_DEF_VALUE -1
#define CFG_INT_FREE(VAR)
#define CFG_INT_ASSIGN(VAR, VAL) VAR = VAL;
#define CFG_INT_PARSE(DICT, VAR, SECTION, NAME) \
    CFG_INT_ASSIGN(VAR, iniparser_getint(DICT, SECTION":"NAME, CFG_INT_DEF_VALUE))

#define CFG_STRING_TYPE char *
#define CFG_STRING_DEF_VALUE NULL
#define CFG_STRING_FREE(VAR) if (VAR) free(VAR);
#define CFG_STRING_ASSIGN(VAR, VAL) VAR = VAL ? strdup(VAL) : CFG_STRING_DEF_VALUE
#define CFG_STRING_PARSE(DICT, VAR, SECTION, NAME) \
    CFG_STRING_ASSIGN(VAR, iniparser_getstring(DICT, SECTION":"NAME, CFG_STRING_DEF_VALUE))


//            var,               section,  name,                type,   def value
#define CFG_GEN                                                                           \
    CFG_ENTRY(port,              "server",       "port",              INT,    80)         \
    CFG_ENTRY(addr,              "server",       "addr",              STRING, "0.0.0.0")  \
    CFG_ENTRY(n_workers,         "server",       "workers",           INT,    2)          \
    CFG_ENTRY(log_level,         "server",       "log_level",         STRING, "LOG_INFO") \
    CFG_ENTRY(path_to_static,    "server",       "path_to_static",    STRING, ".")        \
    CFG_ENTRY(runas,             "server",       "runas",             STRING, NULL)       \
                                                                                          \
    CFG_ENTRY(path_to_css,       "photohosting", "path_to_css",       STRING, ".")        \
    CFG_ENTRY(path_to_store,     "photohosting", "path_to_store",     STRING, ".")        \
    CFG_ENTRY(path_to_tmp_files, "photohosting", "path_to_tmp_files", STRING, ".")        \
                                                                                          \
    CFG_ENTRY(path_to_tokens,    "auth",         "path_to_tokens",    STRING, ".")        \
    CFG_ENTRY(path_to_pwd,       "auth",         "users",             STRING, "./users")


#define CFG_ENTRY(VAR, SECTION, NAME, TYPE, DEF_VALUE) \
    CFG_##TYPE##_TYPE VAR;

#define CFG_ENTRIES CFG_GEN
struct Config
{
    CFG_ENTRIES

    char *path_to_cfg;

#undef CFG_ENTRY
#undef CFG_ENTRIES


    Config();
    ~Config();

    bool Init(const char *path_to_cfg);
    void Check();
};


bool process_cmd_arguments(int argc, char **argv, Config &cfg);

#endif
