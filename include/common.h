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
    char *path_to_tmp_files;
    char *path_to_css;
    char *runas;

    Config();
    ~Config();

    void Check();
};


struct ByteArray
{
    char *data;
    int size;
    int cap;

    ByteArray() : data(NULL), size(0), cap(0) {};
    ByteArray(int _cap): size(0), cap(_cap) { data = (char *)malloc(cap); }
    ByteArray(const char *data, int size);
    ~ByteArray() { if (data) free(data); }

    void Append(const ByteArray *other);
};


bool process_cmd_arguments(int, char **, Config &);
void hexdump(uint8_t *, size_t);
ByteArray *read_file(const char *);
bool file_exists(const char *);
char *gen_random_string(int);
int mkdir_p(const char *, mode_t);
int rm_rf(const char *path);
bool change_user(const char *runas);


#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b


#endif
