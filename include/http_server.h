#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include <sys/types.h>

#include "picohttpparser.h"

#include "http_client.h"


#define MAX_CLIENTS 1024


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


class HttpServer {
    char *addr;
    int port;

    int n_workers;
    Worker *workers;
    bool is_slave;

    int listen_fd;

    SelectOpts so;

    int n_clients;
    HttpClient *clients;

    void Listen();
    void Serve();
    void Wait();

    void AcceptNewConnection();
    void AddNewClient(int, char *);
    void DeleteClient(int);

    bool ParseHttpRequest(HttpClient &);
    void ProcessHeaders(HttpClient &);

    void ProcessRequest(HttpClient &);
    void SendResponse(HttpClient &);

    void RespondOk(HttpClient &);

public:
    HttpServer();
    ~HttpServer();

    void Init();
    void SetArgs(const Config &);

    void ListenAndServe();
};



#endif
