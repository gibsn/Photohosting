#include "users_api.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "exceptions.h"
#include "log.h"


UserNames::UserNames(Photohosting *p)
    : photohosting(p),
    root(NULL),
    curr_it(NULL)
{
    path_to_users = photohosting->GetPathToUsers();
}


void UserNames::Append(char *s)
{
    UserNames::username_node_t *new_node = new UserNames::username_node_t;
    new_node->user_name = strdup(s);
    new_node->next = root;
    root = new_node;
}


void UserNames::Init()
{
    users_dir = opendir(path_to_users);
    if (!users_dir) {
        LOG_E("photohosting: could not open %s: %s", path_to_users, strerror(errno));
        throw GetUsersEx(NULL);
    }

    struct dirent *curr_file;
    while ((curr_file = readdir(users_dir)) != NULL) {
        if (curr_file->d_name[0] == '.') {
            continue;
        }

        Append(curr_file->d_name);
    }
}


const char *UserNames::Next()
{
    if (curr_it) {
        const char *ret = curr_it->user_name;
        curr_it = curr_it->next;
        return ret;
    }

    return NULL;
}


UserNames::~UserNames()
{
    if (users_dir) closedir(users_dir);

    while(root) {
        username_node_t *curr = root;
        root = root->next;
        free(curr->user_name);
        free(curr);
    }
}


AlbumTitles::AlbumTitles(Photohosting *p)
    : photohosting(p),
    user_dir(NULL),
    path_to_user(NULL),
    regexp_compiled(false),
    root(NULL)
{
}


void AlbumTitles::Append(char *s)
{
    AlbumTitles::album_title_node_t *new_node = new AlbumTitles::album_title_node_t;

    new_node->album_title = s;
    new_node->next = root;
    root = new_node;
}


void AlbumTitles::CompileRegexp()
{
    if (int n = regcomp(&preg, "<title>(.*)</title>", REG_EXTENDED)) {
        char errbuf[1024];
        regerror(n, &preg, errbuf, sizeof errbuf);
        LOG_E("photohosting: failed to compile regex: %s", errbuf);
        throw GetAlbumTitlesEx(NULL);
    }

    regexp_compiled = true;
}


void AlbumTitles::Init(const char *user)
{
    if (!photohosting->UserExists(user)) {
        throw UserNotFound(NULL);
    }

    path_to_user = photohosting->GetUserDir(user);
    user_dir = opendir(path_to_user);
    if (!user_dir) {
        LOG_E("photohosting: could not open %s: %s", path_to_user, strerror(errno));
        throw GetAlbumTitlesEx(NULL);
    }

    CompileRegexp();

    struct dirent *curr_file;
    while ((curr_file = readdir(user_dir)) != NULL) {
        if (curr_file->d_name[0] == '.'            ||
            0 == strcmp(curr_file->d_name, "srcs") ||
            0 == strcmp(curr_file->d_name, "thmbs")
        ) {
            continue;
        }

        char *album_title = ParseAlbumTitle(curr_file);
        Append(album_title);
    }
}


char *AlbumTitles::ParseAlbumTitle(struct dirent *dirent)
{
    char *path_to_album = NULL;
    ByteArray *file = NULL;

    try {
        path_to_album = (char *)malloc(strlen(path_to_user) + 1 + dirent->d_namlen + 1);

        strcpy(path_to_album, path_to_user);
        strcat(path_to_album, "/");
        strcat(path_to_album, dirent->d_name);

        file = read_file(path_to_album);
        if (!file) {
            LOG_E("photohosting: could not read file %s", path_to_album);
            throw GetAlbumTitlesEx(NULL);
        }

        file->data[file->size-1] = '\0';

        regmatch_t regmatch[2];
        if (int n = regexec(&preg, file->data, 2, regmatch, 0)) {
            char errbuf[1024];
            regerror(n, &preg, errbuf, sizeof errbuf);
            LOG_E("photohosting: failed to match regex: %s", errbuf);
            throw GetAlbumTitlesEx(NULL);
        }

        off_t match_len = regmatch[1].rm_eo - regmatch[1].rm_so;
        char *title = strndup(file->data + regmatch[1].rm_so, match_len);

        delete file;
        free(path_to_album);

        return title;
    } catch (...) {
        free(path_to_album);
        delete file;
        throw;
    }
}


const char *AlbumTitles::Next()
{
    if (curr_it) {
        const char *ret = curr_it->album_title;
        curr_it = curr_it->next;
        return ret;
    }

    return NULL;
}


AlbumTitles::~AlbumTitles()
{
    if (user_dir) closedir(user_dir);
    free(path_to_user);
    if (regexp_compiled) regfree(&preg);

    while(root) {
        album_title_node_t *curr = root;
        root = root->next;
        free(curr->album_title);
        free(curr);
    }
}


