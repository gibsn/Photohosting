#ifndef USERS_API_H_SENTRY
#define USERS_API_H_SENTRY


#include <dirent.h>
#include <regex.h>

#include "photohosting.h"


class UserNames {
    Photohosting *photohosting;

    DIR *users_dir;
    const char *path_to_users;

    struct username_node_t {
        char *user_name;
        username_node_t *next;
    } *root;

    username_node_t *curr_it;

    void Append(char *s);

public:
    UserNames(Photohosting *p);
    ~UserNames();

    void Init();

    const char *Next();
    void Reset() { curr_it = root; };
};


class AlbumTitles {
    Photohosting *photohosting;

    DIR *user_dir;
    char *path_to_user;

    regex_t preg;
    // calling regfree with uncompiled regex_t causes crash
    bool regexp_compiled;

    struct album_title_node_t {
        char *album_title;
        album_title_node_t *next;
    } *root;

    album_title_node_t *curr_it;

    char *ParseAlbumTitle(struct dirent *dirent);
    void CompileRegexp();
    void Append(char *s);

public:
    AlbumTitles(Photohosting *p);
    ~AlbumTitles();

    void Init(const char *user);

    const char *Next();
    void Reset() { curr_it = root; };
};


#endif
