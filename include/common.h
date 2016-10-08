#ifndef COMMON_H_SENTRY
#define COMMON_H_SENTRY


struct Config
{
    int port;
    char* addr;
    int n_workers;

    Config();
    ~Config();

    void Check();
};


bool process_cmd_arguments(int, char **, Config &);



#endif
