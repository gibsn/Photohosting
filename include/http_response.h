#ifndef HTTP_RESPONSE_H_SENTRY
#define HTTP_RESPONSE_H_SENTRY


#include "common.h"


typedef enum {
    http_continue = 100,
    http_ok = 200,
    http_created = 201,
    http_no_content = 204,
    http_permanent_redirect = 308,
    http_bad_request = 400,
    http_not_found = 404,
    http_internal_error = 500,
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

    bool finalised;


    void AddStatusLine();

    void AddHeader(const char *, const char *);

    //General headers
    void AddDateHeader();
    void AddServerHeader();
    void AddConnectionHeader(bool);

    //Entity headers
    void AddContentLengthHeader();

    void CloseHeaders();

    void AddBody();

public:
    HttpResponse();
    HttpResponse(http_status_t, ByteArray *, int, bool);
    ~HttpResponse();

    ByteArray *GetResponseByteArray();
    ByteArray *GetResponseByteArray(http_status_t);

    void SetBody(ByteArray *);
};


#endif
