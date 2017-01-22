#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include <sys/types.h>

#include "picohttpparser.h"

#include "http_session.h"
#include "tcp_server.h"


struct Config;


class HttpServer: public TcpServer {
    int n_sessions;
    HttpSession **sessions;

    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    void Respond(int, HttpResponse *);

    HttpSession *GetSessionByFd(int);

    virtual void CloseSession(int);
    virtual int CreateNewSession();
    virtual bool ProcessRequest(int);

    char *AddPathToStaticPrefix(char *);

    void ProcessGetRequest(HttpSession *);
    void ProcessPostRequest(HttpSession *);

public:
    HttpServer();
    HttpServer(const Config &);
    virtual ~HttpServer();

    virtual void Init();
    virtual void ListenAndServe();
};



#endif
