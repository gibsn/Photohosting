#ifndef HTTP_REQUEST_H_SENTRY
#define HTTP_REQUEST_H_SENTRY

#include <sys/types.h>

#include "picohttpparser.h"


#define MAX_HTTP_HEADERS 100

struct HttpRequest {
    char *method;
    size_t method_len;

    char *path;
    size_t path_len;

    int minor_version;

    struct phr_header headers[MAX_HTTP_HEADERS];
    int headers_len;
    size_t n_headers;

    char *sid;

    char *if_modified_since;

    char *body;
    int body_len;

    HttpRequest();
    ~HttpRequest();

    void ParseCookie(const char *value, int value_len);

    char *GetMultipartBondary() const;
};



#endif
