#ifndef HTTP_REQUEST_H_SENTRY
#define HTTP_REQUEST_H_SENTRY

#include <sys/types.h>

#include "picohttpparser.h"


#define MAX_HTTP_HEADERS 100

struct HttpRequest {
    bool keep_alive;

    char *method;
    size_t method_len;

    char *path;
    size_t path_len;

    int minor_version;

    struct phr_header headers[MAX_HTTP_HEADERS];
    size_t n_headers;

    char *body;
    int body_len;

    HttpRequest();
    ~HttpRequest();
};



#endif
