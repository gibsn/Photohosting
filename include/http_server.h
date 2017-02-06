#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY


#include "common.h"
#include "tcp_server.h"
#include "tcp_session.h"


struct Config;


class HttpServer: public TcpServer {

    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_tmp_files;

    virtual TcpSession *CreateNewSession();

    char *AddPathToStaticPrefix(const char *) const;

    void ProcessGetRequest();
    void ProcessPostRequest();

public:
    HttpServer();
    HttpServer(const Config &);
    virtual ~HttpServer();

    ByteArray *GetFileByLocation(const char *);

    char *SaveFile(ByteArray *, char *);
};



#endif
