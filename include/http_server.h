#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include "tcp_server.h"


struct stat;

struct Config;
class Photohosting;
class TcpSession;


class HttpServer: public TcpServer {
    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_css; // relatively to path_to_static

    Photohosting *photohosting;

    char *AddPathToStaticPrefix(const char *) const;

    AppLayerDriver *CreateSession(TcpSession *tcp_session);
    void CloseSession(AppLayerDriver *session);

public:
    HttpServer(const Config &cfg, Photohosting *photohosting);
    virtual ~HttpServer();

    virtual bool Init();

    ByteArray *GetFileByPath(const char *path);
    int GetFileStat(const char *path, struct stat *_stat) const;

    Photohosting *GetPhotohosting() { return photohosting; }
};



#endif
