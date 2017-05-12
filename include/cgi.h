#ifndef CGI_H_SENTRY
#define CGI_H_SENTRY


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

public:
    Cgi(const Config &cfg, Photohosting *_photohosting);
    ~Cgi();

    bool Init();
    void Routine();
};

#endif
