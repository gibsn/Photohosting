#ifndef PHOTOHOSTING_H_SENTRY
#define PHOTOHOSTING_H_SENTRY


struct WebAlbumParams;
struct Config;
struct ByteArray;
class AuthDriver;


class Photohosting
{
    AuthDriver *auth;

    char *path_to_store;
    char *path_to_tmp_files;
    char *relative_path_to_css; //relative to the path_to_static

    void _CreateAlbum(const WebAlbumParams &cfg);

public:
    Photohosting(Config &cfg, AuthDriver *auth);
    ~Photohosting();

    bool Init(const Config &cfg);

    const char *GetPathToTmpFiles() const { return path_to_tmp_files; }
    char *GenerateTmpFilePathTemplate();

    char *SaveTmpFile(ByteArray *file);

    char *CreateAlbum(const char *user, const char *archive, const char *title);

    char *Authorise(const char *user, const char *password);
    char *GetUserBySession(const char *sid);
    void Logout(const char *sid);
};


#endif
