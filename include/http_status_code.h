#ifndef HTTP_STATUS_CODE_H_SENTRY
#define HTTP_STATUS_CODE_H_SENTRY


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


#endif
