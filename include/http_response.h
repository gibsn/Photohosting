#ifndef HTTP_RESPONSE_H_SENTRY
#define HTTP_RESPONSE_H_SENTRY


#include <sys/types.h>

#include "picohttpparser.h"


typedef enum {
    http_ok = 200,
    http_permanent_redirect = 308,
    http_bad_request = 400,
    http_not_found = 404,
    http_not_implemented = 501
} http_status_t;


struct HttpResponse {
    char *response;
    int response_len;

    int minor_version;
    http_status_t code;

    bool keep_alive;

    char *body;
    int body_len;

    HttpResponse();
    ~HttpResponse();

    void AddStatusLine();

    void AddHeader(const char *, const char *);

    void AddDefaultHeaders();

    //General headers
    void AddDateHeader();
    void AddServerHeader();
    void AddConnectionHeader(bool);

    //Entity headers
    void AddContentLengthHeader();

    void CloseHeaders();

    void AddBody();
};


#endif
