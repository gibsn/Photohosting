#ifndef COMMON_H_SENTRY
#define COMMON_H_SENTRY

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct ByteArray
{
    char *data;
    int size;
    int cap;

private:
    bool ShouldRealloc(int needed_size) const;
    void Realloc(int new_size);

public:
    ByteArray() : data(NULL), size(0), cap(0) {};
    ByteArray(const char *s);
    ByteArray(const char *s, int size);
    ByteArray(int _cap): size(0), cap(_cap) { data = (char *)malloc(cap); }
    ~ByteArray();

    void Append(const ByteArray *other);
    void Append(const char *s);
    void Append(const char *s, int size);

    void Truncate(uint32_t n);
    void Reset();

    char *GetString() const { return strndup(data, size); }
};


struct Config;

bool process_cmd_arguments(int argc, char **argv, Config &cfg);
void hexdump(uint8_t *, size_t);
ByteArray *read_file(const char *);
bool file_exists(const char *);

char *_gen_random_string(int length);

int mkdir_p(const char *path, mode_t mode);
int rm_rf(const char *path);
bool change_user(const char *runas);


#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b


#endif
