#ifndef TCP_SERVER_H_SENTRY
#define TCP_SERVER_H_SENTRY

#include <string.h>
#include <sys/types.h>

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


class TcpServer {
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

    void AddNewClient(int, char *);
    TcpSession *GetSessionByFd(int);

    virtual bool ProcessRequest(int);

    void ProcessRead(const fd_set &, TcpSession *);
    void ProcessWrite(const fd_set &, TcpSession *);

protected:
    virtual int CreateNewSession();
    virtual void CloseSession(int);

public:
    TcpServer();
    TcpServer(const Config &);
    virtual ~TcpServer();

    const char *GetAddr() const { return addr; }
    int GetPort() const { return port; }

    void SetAddr(const char *a) { addr = strdup(a); }
    void SetPort(int p) { port = p; }
    void SetWorkersCount(int n) { n_workers = n; }

    void WantToCloseSession(int);

    void WriteTo(int, char *, int);
    char *ReadFrom(int);

    virtual void Init();
    virtual void ListenAndServe();
};

#endif
