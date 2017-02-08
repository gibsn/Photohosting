#ifndef HTTP_SESSION_H_SENTRY
#define HTTP_SESSION_H_SENTRY

#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "tcp_session.h"


typedef enum {
    ok = 0,
    incomplete_request = -1,
    invalid_request = -2
} request_parser_result_t;


class HttpSession: public TcpSessionDriver
{
    HttpServer *http_server;

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

    void Respond(http_status_t);

    void RespondStatic(const char *);

    char *UploadFile(const char *);
    http_status_t CreateWebAlbum();

    ByteArray *GetFileFromRequest(char **) const;

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
