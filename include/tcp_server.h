#ifndef TCP_SERVER_H_SENTRY
#define TCP_SERVER_H_SENTRY

#include <string.h>
#include <sys/types.h>

#include "select_loop_driver.h"
#include "tcp_session.h"

#define MAX_SESSIONS 1024



struct Config;


struct SelectOpts {
    int nfds;
    fd_set readfds;
    fd_set writefds;

    SelectOpts();
};


struct Worker {
    pid_t pid;
};


class TcpServer: public SelectLoopDriver {
    char *addr;
    int port;

    int n_workers;
    Worker *workers;
    bool is_slave;

    int listen_fd;

    SelectOpts so;

    int n_sessions;
    TcpSession **sessions;

    void Listen();
    void Serve();
    void Wait();

    virtual void CloseSession(TcpSession *session);

    void ProcessRead(const fd_set &readfds, TcpSession *session);
    void ProcessWrite(const fd_set &writefds, TcpSession *session);

    void RequestWriteForFd(int fd) { FD_SET(fd, &so.writefds); }

protected:
    virtual TcpSession *CreateNewSession();

public:
    TcpServer();
    TcpServer(const Config &cfg);
    virtual ~TcpServer();

    const char *GetAddr() const { return addr; }
    int GetPort() const { return port; }

    void SetAddr(const char *a) { addr = strdup(a); }
    void SetPort(int p) { port = p; }
    void SetWorkersCount(int n) { n_workers = n; }

    virtual void Init();
    virtual void ListenAndServe();
};

#endif
