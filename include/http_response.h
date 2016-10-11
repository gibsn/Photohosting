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
    char *response;
    int response_len;

    int minor_version;
    http_status_t code;

    char *body;
    int body_len;

    HttpResponse();
    ~HttpResponse();

    void AddStatusLine();

    void AddHeader(const char *, const char *);

    //Genereal headers
    void AddDateHeader();
    void AddConnectionHeader(bool);

    //Entity headers
    void AddContentLengthHeader();

    void CloseHeaders();

    void AddBody();
};


#endif
