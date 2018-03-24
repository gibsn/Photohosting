#ifndef TCP_SERVER_H_SENTRY
#define TCP_SERVER_H_SENTRY

#include <string.h>
#include <sys/types.h>

extern "C" {
#include "sue_base.h"
}

#include "select_loop_driver.h"
#include "tcp_session.h"

#define MAX_FD 1024


struct Config;


struct Worker {
    pid_t pid;
};


class TcpServer {
    char *addr;
    int port;

    int n_workers;
    Worker *workers;
    bool is_slave;

    struct sue_event_selector selector;
    struct sue_fd_handler listen_fd_h;

    int n_sessions;
    TcpSession **sessions;

    bool Listen();
    void Serve();
    void Wait();

    static void ListenFdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x);

    static void FdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x);
    void FdHandler(TcpSession *session, int r, int w, int x);

    static void ToutHandlerCb(sue_timeout_handler *tout_h);
    void ToutHandler(TcpSession *session);

    TcpSession *CreateTcpSession();
    virtual AppLayerDriver *CreateSession(TcpSession *tcp_session);

    void CloseTcpSession(TcpSession *tcp_session);
    virtual void CloseSession(AppLayerDriver *session);

    void ProcessRead(TcpSession *session);
    void ProcessWrite(TcpSession *session);

public:
    TcpServer();
    TcpServer(const Config &cfg);
    virtual ~TcpServer();

    const char *GetAddr() const { return addr; }
    int GetPort() const { return port; }

    void SetAddr(const char *a) { addr = strdup(a); }
    void SetPort(int p) { port = p; }
    void SetWorkersCount(int n) { n_workers = n; }

    virtual bool Init();
    virtual bool ListenAndServe();
};

#endif
