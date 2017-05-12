#ifndef CGI_H_SENTRY
#define CGI_H_SENTRY

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

    // all these close headers
    void SetStatus(http_status_t status);
    void SetContentType(const char *type);
    void SetCookie(const char *key, const char *value);

    void CloseHeaders();

    void Respond(const char *text);

public:
    Cgi(const Config &cfg, Photohosting *_photohosting);
    ~Cgi();

    bool Init();
    void Routine();
};

#endif
