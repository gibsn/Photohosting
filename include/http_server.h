#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include "tcp_server.h"


struct Config;
class Photohosting;
class TcpSession;


class HttpServer: public TcpServer {
    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_tmp_files;
    char *path_to_css; // relatively to path_to_static

    Photohosting *photohosting;

    virtual TcpSession *CreateNewSession();

    char *AddPathToStaticPrefix(const char *) const;

public:
    HttpServer(const Config &cfg, Photohosting *photohosting);
    virtual ~HttpServer();

    char *SaveFile(ByteArray *, char *);

    ByteArray *GetFileByLocation(const char *);
    Photohosting *GetPhotohosting() { return photohosting; }
};



#endif
