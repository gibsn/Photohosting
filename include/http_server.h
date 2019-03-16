#ifndef HTTP_SERVER_H_SENTRY
#define HTTP_SERVER_H_SENTRY

#include "application.h"

extern "C" {
#include "sue_base.h"
}


struct ByteArray;
struct Config;
class Photohosting;
struct stat;
class TransportSessionBridge;


class HttpServer: public ApplicationLevelSessionManagerBridge {
    sue_event_selector &selector;

    char *path_to_static;
    int path_to_static_len; // not to use strlen every time

    char *path_to_css; // relatively to path_to_static

    Photohosting *photohosting;

    char *AddPathToStaticPrefix(const char *) const;

    ApplicationLevelBridge *Create(TransportSessionBridge *t_session);
    void Close(ApplicationLevelBridge *session);

public:
    HttpServer(const Config &cfg, sue_event_selector &selector, Photohosting *photohosting);
    virtual ~HttpServer();

    virtual bool Init();

    ByteArray *GetFileByPath(const char *path);
    int GetFileStat(const char *path, struct stat *_stat) const;

    Photohosting *GetPhotohosting() { return photohosting; }
};



#endif
