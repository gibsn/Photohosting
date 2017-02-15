#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY


#include "tcp_server.h"
#include "tcp_session.h"



struct Config;
class AuthDriver;


class HttpServer: public TcpServer {

    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_tmp_files;

    char *path_to_css; // relatively to path_to_static

    AuthDriver *auth;


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
    char *CreateAlbum(char *, char *, char *, char **);
};



#endif
