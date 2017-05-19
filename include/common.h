#ifndef COMMON_H_SENTRY
#define COMMON_H_SENTRY

#include <stdint.h>
#include <stdlib.h>


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


struct Config;

bool process_cmd_arguments(int argc, char **argv, Config &cfg);
void hexdump(uint8_t *, size_t);
ByteArray *read_file(const char *);
bool file_exists(const char *);

//this will come from WebAlbumCreator
char *_gen_random_string(int length);

int mkdir_p(const char *, mode_t);
int rm_rf(const char *path);
bool change_user(const char *runas);


#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b


#endif
