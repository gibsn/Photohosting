#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY


struct Config;


struct SelectOpts
{
};


class HttpServer
{
    char *addr;
    int port;
    int n_workers;

    bool slave;
    int listen_fd;

    SelectOpts select_opts;

    //array of clients;

    bool ProcessRequest();
    bool SendResponse();
    void Listen();
    void Serve();
    void Wait();

public:
    HttpServer();
    ~HttpServer();

    void SetArgs(const Config &);

    void ListenAndServe();
};





#endif
