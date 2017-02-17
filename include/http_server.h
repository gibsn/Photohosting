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
    HttpServer(const Config &cfg, AuthDriver *_auth);
    virtual ~HttpServer();

    ByteArray *GetFileByLocation(const char *);

    char *SaveFile(ByteArray *, char *);

    char *CreateAlbum(
            const char *user,
            const char *archive,
            const char *page_title,
            char **album_path);

    //Auth
    char *Authorise(const char *user, const char *password);
    const char *GetUserBySession(const char *sid);
};



#endif
