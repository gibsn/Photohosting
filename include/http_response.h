#ifndef HTTP_RESPONSE_H_SENTRY
#define HTTP_RESPONSE_H_SENTRY

#include "common.h"
#include "http_status_code.h"


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

    //Response headers
    void AddLocationHeader(const char *loc);
    void SetCookie(const char *key, const char *value);

    //Entity headers
    void AddContentLengthHeader();

    void CloseHeaders();

    void AddBody();

public:
    HttpResponse();
    HttpResponse(http_status_t code, int minor_version, bool keep_alive);
    ~HttpResponse();

    ByteArray *GetResponseByteArray();
    ByteArray *GetResponseByteArray(http_status_t);

    void SetBody(ByteArray *buf);
    void SetBody(const char *buf);

    void AddDefaultBodies();
};


#endif
