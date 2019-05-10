#ifndef PHOTOHOSTING_H_SENTRY
#define PHOTOHOSTING_H_SENTRY


#include "grants.h"


struct WebAlbumParams;
struct Config;
struct ByteArray;
class AuthDriver;


class PhotohostingGrantsBuilder: public GrantsBuilder {
    char *album;
    char *photo;


    virtual char *GrantParamsToPath() const;

public:
    PhotohostingGrantsBuilder(const char *path_to_grants);
    ~PhotohostingGrantsBuilder();

    void SetAlbum(const char *_album) {
        free(album);
        album = strdup(_album);
    }
    void ResetAlbum() {
        free(album);
        album = NULL;
    }
    void SetPhoto(const char *_photo) {
        free(photo);
        photo = strdup(_photo);
    }
    void ResetPhoto() {
        free(photo);
        photo = NULL;
    }
};

class PhotohostingGrants: public Grants{

public:
    PhotohostingGrants(const char *path_to_grants);
    ~PhotohostingGrants();

    void Init() const;
    PhotohostingGrantsBuilder *Builder();
};

class Photohosting
{
    AuthDriver *auth;
    PhotohostingGrants *grants;

    char *path_to_store;
    char *path_to_users;
    char *path_to_tmp_files;
    char *relative_path_to_css; //relative to the path_to_static

    void _CreateAlbum(const WebAlbumParams &cfg);

public:
    Photohosting(Config &cfg, AuthDriver *auth);
    ~Photohosting();

    bool Init(const Config &cfg);

    const char *GetPathToTmpFiles() const { return path_to_tmp_files; }
    const char *GetPathToUsers() const { return path_to_users; }
    char *GetUserDir(const char *user) const;

    char *GenerateTmpFilePathTemplate();
    char *SaveTmpFile(ByteArray *file);

    char *CreateAlbum(const char *user, const char *archive, const char *title);

    char *Authorise(const char *user, const char *password);
    char *GetUserBySession(const char *sid);
    void Logout(const char *sid);

    bool UserExists(const char *user) const;
};


#endif
