#ifndef COMMON_H_SENTRY
#define COMMON_H_SENTRY

#include <stdint.h>
#include <stdlib.h>


struct Config
{
    int port;
    char *addr;
    int n_workers;
    int max_log_level;
    char *path_to_static;

    Config();
    ~Config();

    void Check();
};


struct ByteArray
{
    char *data;
    int size;

    ByteArray() : data(NULL), size(0) {};
    ByteArray(char *, int);
    ~ByteArray() { if (data) free(data); }

    void Append(const ByteArray *);
};


bool process_cmd_arguments(int, char **, Config &);
void hexdump(uint8_t *, size_t);
ByteArray *read_file(const char *);
bool file_exists(const char *);



#endif
