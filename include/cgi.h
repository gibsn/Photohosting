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
    CGI_varlist *query_list;
    CGI_varlist *cookies;
    CGI_varlist *post_body;

    char *user;
    char *album_path;

    Photohosting *photohosting;

    void ProcessGetRequest(CGI_value query);
    void ProcessPostRequest(CGI_value query);

    void ProcessListUsers();
    void ProcessListAlbums();

    void ProcessUploadPhotos();
    void ProcessLogin();
    void ProcessLogout();

    char *CreateWebAlbum(const char *user);

    const char *GetSidFromCookies();
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
