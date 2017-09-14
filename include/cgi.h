#ifndef CGI_H_SENTRY
#define CGI_H_SENTRY

extern "C" {
#include "ccgi.h"
}

#include "http_status_code.h"


struct Config;
class Photohosting;


class Cgi
{
    Photohosting *photohosting;

    void ProcessGetRequest();
    void ProcessPostRequest();

    void ProcessUploadPhotos();
    void ProcessLogin();
    void ProcessLogout();

    char *CreateWebAlbum(const char *user);

    char *GetSidFromCookies();
    char *GetUserBySid(const char *sid);

    // these close headers
    void SetStatus(http_status_t status);
    void SetContentType(const char *type);

    // these dont
    void SetCookie(const char *key, const char *value);
    void SetLocation(const char *location);

    void CloseHeaders();

    void Respond(const char *text);

public:
    Cgi(const Config &cfg, Photohosting *_photohosting);
    ~Cgi();

    bool Init();
    void Routine();
};

#endif
