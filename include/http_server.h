#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include <sys/types.h>

#define MAX_CLIENTS 1024


struct Config;


struct SelectOpts
{
    int nfds;
    fd_set readfds;
    fd_set writefds;

    SelectOpts();
};


struct Worker
{
    pid_t pid;
};


struct Client
{
    int fd;

    char *read_buf;
    char *write_buf;

    char *s_addr;

    Client();
    ~Client();

    void Free();
};


class HttpServer
{
    char *addr;
    int port;

    int n_workers;
    Worker *workers;
    bool is_slave;

    int listen_fd;

    SelectOpts so;

    int n_clients;
    Client *clients;

    void Listen();
    void Serve();
    void Wait();

    void AcceptNewConnection();
    void AddNewClient(int, char *);
    void DeleteClient(int);

    bool ProcessRequest(Client &);
    bool SendResponse(Client &);

public:
    HttpServer();
    ~HttpServer();

    void Init();
    void SetArgs(const Config &);

    void ListenAndServe();
};





#endif
