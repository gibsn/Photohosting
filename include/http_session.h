#ifndef HTTP_SESSION_H_SENTRY
#define HTTP_SESSION_H_SENTRY

#include "app_layer_driver.h"


class Photohosting;
class HttpServer;
struct HttpRequest;
struct HttpResponse;
class TcpSession;
struct ByteArray;

typedef enum {
    ok = 0,
    incomplete_request = -1,
    invalid_request = -2
} request_parser_result_t;


class HttpSession: public AppLayerDriver {
    HttpServer *http_server;
    Photohosting *photohosting;

    char *s_addr;

    ByteArray *read_buf;

    bool active;
    TcpSession *tcp_session;

    HttpRequest *request;
    HttpResponse *response;

    bool keep_alive;

    request_parser_result_t ParseHttpRequest(ByteArray *);
    void ProcessHeaders();
    void PrepareForNextRequest();

    bool ValidateLocation(char *);

    void ProcessGetRequest();
    void ProcessPostRequest();

    void Respond();

    void ProcessLogin();
    void ProcessLogout();

    void ProcessPhotosUpload();
    void ProcessStatic(const char *);

    char *UploadFile(const char *);
    char *CreateWebAlbum(const char *user, const char *page_title);

    ByteArray *GetFileFromRequest() const;

public:
    HttpSession(TcpSession *, HttpServer *);
    ~HttpSession();

    virtual bool ProcessRequest();
    virtual void Close();

    const HttpRequest *GetRequest() const { return request; }
    const bool GetKeepAlive() const { return keep_alive; }

    void SetKeepAlive(bool b) { keep_alive = b; }
};

#endif
