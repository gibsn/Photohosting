#ifndef HTTP_SESSION_H_SENTRY
#define HTTP_SESSION_H_SENTRY

#include "http_request.h"
#include "http_response.h"



class HttpSession {
    int fd;

    HttpRequest *request;
    HttpResponse *response;

    bool keep_alive;

public:
    HttpSession();
    HttpSession(int);
    ~HttpSession();

    char *RespondAny();

    //2XX
    char *RespondOk();

    //3XX
    char *RespondPermanentRedirect(const char *);

    //4XX
    char *RespondNotFound();

    //5XX
    char *RespondNotImplemented();

    bool ParseHttpRequest(char *);
    void ProcessHeaders();
    void PrepareForNextRequest();

    const HttpRequest *GetRequest() const { return request; }
    int GetFd() const { return fd; }
    const bool GetKeepAlive() const { return keep_alive; }

    void SetKeepAlive(bool b) { keep_alive = b; }
};

#endif
