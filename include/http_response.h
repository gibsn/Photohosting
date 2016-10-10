#ifndef HTTP_RESPONSE_H_SENTRY
#define HTTP_RESPONSE_H_SENTRY


#include <sys/types.h>

#include "picohttpparser.h"


typedef enum {
    http_ok = 200,
    http_bad_request = 400,
    http_not_found = 404
} http_status_t;


struct HttpResponse {
    int minor_version;
    http_status_t code;

    struct HeadersList {
        struct phr_header header;
        struct HeadersList *next;
    } *headers_list;
    size_t n_headers;

    char *body;
    int body_len;

    HttpResponse();
    ~HttpResponse();

    void AddHeader(const char *, const char *);
    void AddDateHeader();

    char *HeaderToStr(const phr_header &);
    char *GenerateResponse(int &);
};


#endif
