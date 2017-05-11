#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY


#include "tcp_server.h"
#include "tcp_session.h"


struct Config;
class AuthDriver;
class Photohosting;


class HttpServer: public TcpServer {
    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_tmp_files;

    char *path_to_css; // relatively to path_to_static

    Photohosting *photohosting;
    AuthDriver *auth;

    virtual TcpSession *CreateNewSession();

    char *AddPathToStaticPrefix(const char *) const;

public:
    HttpServer(const Config &cfg, Photohosting *photohosting, AuthDriver *auth);
    virtual ~HttpServer();

    ByteArray *GetFileByLocation(const char *);

    char *SaveFile(ByteArray *, char *);

    Photohosting *GetPhotohosting() { return photohosting; }

    char *Authorise(const char *user, const char *password);
    char *GetUserBySession(const char *sid);
    void Logout(const char *sid);
};



#endif
