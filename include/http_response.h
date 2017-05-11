#ifndef HTTP_RESPONSE_H_SENTRY
#define HTTP_RESPONSE_H_SENTRY


#include "common.h"


//                      int  enum                       status line                  def body
#define HTTP_RESPONSE_GEN \
    HTTP_RESPONSE_ENTRY(100, http_continue,             "100 CONTINUE",              NULL)                                               \
    HTTP_RESPONSE_ENTRY(200, http_ok,                   "200 OK",                    "OK")                                               \
    HTTP_RESPONSE_ENTRY(201, http_created,              "201 CREATED",               NULL)                                               \
    HTTP_RESPONSE_ENTRY(204, http_no_content,           "204 NO CONTENT",            NULL)                                               \
    HTTP_RESPONSE_ENTRY(303, http_see_other,            "303 SEE OTHER",             NULL)                                               \
    HTTP_RESPONSE_ENTRY(308, http_permanent_redirect,   "308 PERMANENT REDIRECT",    NULL)                                               \
    HTTP_RESPONSE_ENTRY(400, http_bad_request,          "400 BAD REQUEST",           "Bad request")                                      \
    HTTP_RESPONSE_ENTRY(401, http_unauthorized,         "401 UNAUTHORIZED",          "You are not allowed to do this, please authorise") \
    HTTP_RESPONSE_ENTRY(403, http_forbidden,            "401 FORBIDDEN",             "You are not allowed to do this, please authorise") \
    HTTP_RESPONSE_ENTRY(404, http_not_found,            "404 NOT FOUND",             "Not found")                                        \
    HTTP_RESPONSE_ENTRY(500, http_internal_error,       "500 INTERNAL SERVER ERROR", "Internal server error")                            \
    HTTP_RESPONSE_ENTRY(501, http_not_implemented,      "501 NOT IMPLEMENTED",       NULL)                                               \
    HTTP_RESPONSE_ENTRY(507, http_insufficient_storage, "507 INSUFFICIENT STORAGE", "Insufficient storage")


#define HTTP_RESPONSE_ENTRY(INT, ENUM, STATUS, BODY)\
    ENUM = INT,

typedef enum {
    HTTP_RESPONSE_GEN
} http_status_t;

#undef HTTP_RESPONSE_ENTRY


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
