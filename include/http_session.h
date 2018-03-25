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


class HttpSession: public AppLayerDriver {
    enum {
        state_idle = 0,
        state_waiting_for_headers,
        state_processing_headers,
        state_waiting_for_body,
        state_processing_request,
        state_responding,
        state_shutting_down,
    } state;
    bool should_wait_for_more_data;

    HttpServer *http_server;
    Photohosting *photohosting;

    char *s_addr;

    ByteArray read_buf;
    uint32_t last_headers_len;

    TcpSession *tcp_session;

    HttpRequest request;
    HttpResponse response;

    bool keep_alive;

    // Finite state machine
    void ProcessStateIdle();
    void ProcessStateWaitingForHeaders();
    void ProcessStateProcessingHeaders();
    void ProcessStateWaitingForBody();
    void ProcessStateProcessingRequest();
    void ProcessStateResponding();
    void ProcessStateShuttingDown();

    void ProcessHeaders();

    bool ValidateLocation(char *path);

    void ProcessGetRequest();
    void ProcessPostRequest();

    void InitHttpResponse(http_status_t status);
    void Respond();
    void Reset();

    // API
    void ProcessLogin();
    void ProcessLogout();

    void ProcessPhotosUpload();
    void ProcessStatic(const char *);

    char *UploadFile(const char *);

    ByteArray *GetFileFromRequest() const;

    void SetWantToClose() { keep_alive = false; }

public:
    HttpSession(TcpSession *, HttpServer *);
    ~HttpSession();

    virtual void ProcessRequest();
    void Close();
};

#endif
