#ifndef TCP_CLIENT_H_SENTRY
#define TCP_CLIENT_H_SENTRY

#include <stdint.h>

extern "C" {
#include "sue_base.h"
}

#include "tcp_session.h"

struct ByteArray;


class TcpClient: public TcpSessionManagerBridge, public TransportClientBridge
{
    struct sue_event_selector &selector;
    TcpSession *tcp_session;


    static char *AddrFromHostPort(const char *host, uint16_t port);

    static void FdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x);
    void FdHandler(TcpSession *session, int r, int w, int x);

    static void TimeoutHandlerCb(sue_timeout_handler *timeout_h);
    void TimeoutHandler(TcpSession *session);

public:
    TcpClient(sue_event_selector &selector);
    ~TcpClient();

    void Dial(const char *addr, uint16_t port, ApplicationLevelSessionManagerBridge *asb);
    void Shutdown();

    ByteArray *Read();
    void Write(ByteArray *buf);

    void OnRead(TcpSession *session);
    void OnWrite(TcpSession *session);
    void OnX(TcpSession *session);
    void OnTimeout(TcpSession *session);
};

#endif
