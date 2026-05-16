#ifndef HTTP_CLIENT_H_SENTRY
#define HTTP_CLIENT_H_SENTRY


#include "tcp_session.h"
#include "tcp_client.h"
#include "transport.h"


class HttpClient: public ApplicationLevelBridge, public ApplicationLevelSessionManagerBridge
{
    enum {
        http_client_idle,
        http_client_waiting_get_response,
    } state;

    TransportClientBridge *t_client;

    void ProcessWaitingPostResponse();

public:
    HttpClient(struct sue_event_selector &selector);
    ~HttpClient();

    void OnRead();
    void OnWrite();

    virtual ApplicationLevelBridge *Create(TransportSessionBridge *t) = 0;
    virtual void Close(ApplicationLevelBridge *h) = 0;

    void Dial(const char *addr);
    void Post(const char *path, const char *params);
};

#endif
