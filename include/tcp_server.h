#ifndef TCP_SERVER_H_SENTRY
#define TCP_SERVER_H_SENTRY

#include <string.h>
#include <sys/types.h>

extern "C" {
#include "sue_base.h"
}

#include "tcp_session.h"

#define MAX_FD 1024


struct Config;


struct Worker {
    pid_t pid;
};


class TcpServer: public TcpSessionManagerBridge
{
    char *addr;
    int port;

    int n_workers;
    Worker *workers;
    bool is_slave;

    struct sue_event_selector &selector;
    struct sue_fd_handler listen_fd_h;

    ApplicationLevelSessionManagerBridge *application_level_session_manager;

    int n_sessions;
    TcpSession **sessions;

    bool SpawnWorkers();
    void Wait();

    static void ListenFdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x);

    TcpSession *CreateTcpSession();
    void CloseTcpSession(TcpSession *tcp_session);

    void ProcessRead(TcpSession *session);
    void ProcessWrite(TcpSession *session);

public:
    TcpServer();
    TcpServer(const Config &cfg, sue_event_selector &selector);
    virtual ~TcpServer();

    const char *GetAddr() const { return addr; }
    void SetAddr(const char *a) { addr = strdup(a); }

    int GetPort() const { return port; }
    void SetPort(int p) { port = p; }

    void SetWorkersCount(int n) { n_workers = n; }

    ApplicationLevelSessionManagerBridge *GetApplicationLevelSessionManager() {
        return application_level_session_manager;
    }
    void SetApplicationLevelSessionManager(ApplicationLevelSessionManagerBridge *m) {
        application_level_session_manager = m;
    }

    void OnRead(TcpSession *session);
    void OnWrite(TcpSession *session);
    void OnX(TcpSession *session);
    void OnTimeout(TcpSession *session);

    virtual bool Init();
};

#endif
