#ifndef HTTP_SESSION_H_SENTRY
#define HTTP_SESSION_H_SENTRY

#include "app_layer_driver.h"
#include "common.h"
#include "http_request.h"
#include "http_response.h"
#include "http_status_code.h"


class Photohosting;
class HttpServer;
class TcpSession;

typedef enum {
    ok = 0,
    invalid_request = -1,
    incomplete_request = -2,
} request_parser_result_t;


class HttpSession: public AppLayerDriver {
    HttpServer *http_server;
    Photohosting *photohosting;

    char *s_addr;

    ByteArray read_buf;
    uint32_t last_len;

    TcpSession *tcp_session;

    HttpRequest request;
    HttpResponse response;

    bool keep_alive;

    request_parser_result_t ParseHttpRequest();
    void ProcessHeaders();
    void PrepareForNextRequest();

    bool ValidateLocation(char *path);

    void ProcessGetRequest();
    void ProcessPostRequest();

    void InitHttpResponse(http_status_t status);
    void Respond();
    void Reset();

    void ProcessLogin();
    void ProcessLogout();

    void ProcessPhotosUpload();
    void ProcessStatic(const char *);

    char *UploadFile(const char *);

    ByteArray *GetFileFromRequest() const;

public:
    HttpSession(TcpSession *, HttpServer *);
    ~HttpSession();

    virtual void ProcessRequest();
    void Close();
};

#endif
