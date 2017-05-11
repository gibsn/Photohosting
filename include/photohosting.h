#ifndef PHOTOHOSTING_H_SENTRY
#define PHOTOHOSTING_H_SENTRY


struct WebAlbumParams;
class AuthDriver;


class Photohosting
{
    AuthDriver *auth;

    char *path_to_static;
    char *relative_path_to_css; //relative to the path_to_static

    void _CreateAlbum(const WebAlbumParams &cfg);

public:
    Photohosting(const char *_path_to_static, const char *_relative_path_to_css, AuthDriver *auth);
    ~Photohosting();

    char *CreateAlbum(const char *user, const char *archive, const char *title);

    char *Authorise(const char *user, const char *password);
    char *GetUserBySession(const char *sid);
    void Logout(const char *sid);
};


#endif
