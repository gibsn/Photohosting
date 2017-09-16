#ifndef PHOTOHOSTING_H_SENTRY
#define PHOTOHOSTING_H_SENTRY


#include <string.h>
#include <stdlib.h>


struct WebAlbumParams;
struct Config;
struct ByteArray;
class AuthDriver;


class User
{
    char *name;

public:
    User(const char *s) { name = strdup(s); }
    ~User() { free(name); }

    const char *GetName() { return name; }
};


struct Users {
    struct username_node_t {
        User *user;
        username_node_t *next;
    } *root;

    Users(): root(NULL) {};

    // list is supposed to be freed manually
    ~Users() {};
};


class Photohosting
{
    AuthDriver *auth;

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
    char *GenerateTmpFilePathTemplate();

    char *SaveTmpFile(ByteArray *file);

    char *CreateAlbum(const char *user, const char *archive, const char *title);

    char *Authorise(const char *user, const char *password);
    char *GetUserBySession(const char *sid);
    void Logout(const char *sid);

    Users GetUsers();
};


#endif
