#ifndef HTTP_SESSION_H_SENTRY
#define HTTP_SESSION_H_SENTRY

#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "tcp_session.h"


class HttpSession: public TcpSessionDriver
{
    HttpServer *http_server;

    bool active;
    TcpSession *tcp_session;

    HttpRequest *request;
    HttpResponse *response;

    bool keep_alive;

    // TODO: this function should return more than bool
    bool ParseHttpRequest(char *);
    void ProcessHeaders();
    void PrepareForNextRequest();

    bool ValidateLocation(char *);

    void ProcessGetRequest();
    void ProcessPostRequest();

    void Respond(http_status_t);

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
