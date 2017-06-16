#include "web_album_creator_helper.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "WebAlbumCreator.h"

#include "common.h"
#include "log.h"


char *make_user_path(const char *path_to_store, const char *user)
{
    int user_path_len = strlen(path_to_store) + sizeof "/users/" - 1;
    user_path_len += strlen(user) + sizeof "/";
    char *user_path = (char *)malloc(user_path_len);

    strcpy(user_path, path_to_store);
    strcat(user_path, "/users/");
    strcat(user_path, user);
    strcat(user_path, "/");

    return user_path;
}


static char *make_path_to_unpack(const char *user_path, const char *random_id)
{
    int path_to_unpack_len = strlen(user_path) + sizeof "srcs/" - 1;
    path_to_unpack_len += strlen(random_id) + sizeof "/";
    char *path_to_unpack = (char *)malloc(path_to_unpack_len);

    strcpy(path_to_unpack, user_path);

    strcat(path_to_unpack, "srcs/");

    strcat(path_to_unpack, random_id);
    strcat(path_to_unpack, "/");

    return path_to_unpack;
}


static char *make_path_to_thumbs(const char *user_path, const char *random_id)
{
    int path_to_thumbs_len = strlen(user_path) + sizeof "thmbs/" - 1;
    path_to_thumbs_len += strlen(random_id) + sizeof "/";
    char *path_to_thumbs = (char *)malloc(path_to_thumbs_len);

    strcpy(path_to_thumbs, user_path);

    strcat(path_to_thumbs, "thmbs/");

    strcat(path_to_thumbs, random_id);
    strcat(path_to_thumbs, "/");

    return path_to_thumbs;
}


static char *make_path_to_webpage(const char *user_path, const char *random_id)
{
    int path_to_webpage_len = strlen(user_path) + sizeof "/" - 1;
    path_to_webpage_len += strlen(random_id) + sizeof ".html";
    char *path_to_webpage = (char *)malloc(path_to_webpage_len);

    strcpy(path_to_webpage, user_path);

    strcat(path_to_webpage, random_id);
    strcat(path_to_webpage, ".html");

    return path_to_webpage;
}


char *make_path_to_css(const char *path_to_css)
{
    int path_to_css_file_len = sizeof "/static/" - 1;
    path_to_css_file_len += strlen(path_to_css) + sizeof "/blue.css";
    char *path_to_css_file = (char *)malloc(path_to_css_file_len);

    strcpy(path_to_css_file, "/static/");

    strcat(path_to_css_file, path_to_css);
    strcat(path_to_css_file, "/blue.css");

    return path_to_css_file;
}


static char *make_r_path_to_srcs(const char *user, const char *random_id)
{
    int r_path_to_srcs_len = sizeof "/static/" - 1 + sizeof "users/" - 1 + strlen(user);
    r_path_to_srcs_len +=  sizeof "/srcs/" - 1 + strlen(random_id) + sizeof "/";
    char *r_path_to_srcs = (char *)malloc(r_path_to_srcs_len);

    strcpy(r_path_to_srcs, "/static/");

    strcat(r_path_to_srcs, "users/");
    strcat(r_path_to_srcs, user);
    strcat(r_path_to_srcs, "/srcs/");
    strcat(r_path_to_srcs, random_id);
    strcat(r_path_to_srcs, "/");

    return r_path_to_srcs;
}


static char *make_r_path_to_thmbs(const char *user, const char *random_id)
{
    int r_path_to_thmbs_len = sizeof "/static/" - 1 + sizeof "users/" - 1 + strlen(user);
    r_path_to_thmbs_len +=  sizeof "/thmbs/" - 1 + strlen(random_id) + sizeof "/";

    char *r_path_to_thmbs = (char *)malloc(r_path_to_thmbs_len);

    strcpy(r_path_to_thmbs, "/static/");

    strcat(r_path_to_thmbs, "users/");
    strcat(r_path_to_thmbs, user);
    strcat(r_path_to_thmbs, "/thmbs/");
    strcat(r_path_to_thmbs, random_id);
    strcat(r_path_to_thmbs, "/");

    return r_path_to_thmbs;
}


char *make_r_path_to_webpage(const char *user, const char *random_id)
{
    int r_path_to_webpage_len = sizeof "/static/" - 1 + sizeof "users/" - 1 + strlen(user);
    r_path_to_webpage_len +=  sizeof "/" - 1 + strlen(random_id) + sizeof ".html";

    char *r_path_to_webpage = (char *)malloc(r_path_to_webpage_len);

    strcpy(r_path_to_webpage, "/static/");

    strcat(r_path_to_webpage, "users/");
    strcat(r_path_to_webpage, user);
    strcat(r_path_to_webpage, "/");
    strcat(r_path_to_webpage, random_id);
    strcat(r_path_to_webpage, ".html");

    return r_path_to_webpage;
}


void album_creator_debug(const WebAlbumParams &cfg)
{
    LOG_D("path_to_archive: %s", cfg.path_to_archive);
    LOG_D("web_page_title: %s", cfg.web_page_title);
    LOG_D("path_to_srcs: %s", cfg.path_to_unpack);
    LOG_D("path_to_thumbs: %s", cfg.path_to_thumbnails);
    LOG_D("path_to_webpage: %s", cfg.path_to_webpage);
    LOG_D("path_to_css: %s", cfg.path_to_css);
    LOG_D("relative_path_to_srcs: %s", cfg.relative_path_to_originals);
    LOG_D("relative_path_to_thmbs: %s", cfg.relative_path_to_thumbnails);
}


WebAlbumParams album_params_helper(
        const char *user,
        const char *user_path,
        const char *random_id)
{
    WebAlbumParams cfg;

    cfg.path_to_unpack = make_path_to_unpack(user_path, random_id);
    cfg.path_to_thumbnails = make_path_to_thumbs(user_path, random_id);
    cfg.path_to_webpage = make_path_to_webpage(user_path, random_id);


    cfg.relative_path_to_originals = make_r_path_to_srcs(user, random_id);
    cfg.relative_path_to_thumbnails = make_r_path_to_thmbs(user, random_id);

    return cfg;
}


void free_album_params(WebAlbumParams &cfg)
{
    free((char *)cfg.path_to_unpack);
    free((char *)cfg.path_to_thumbnails);
    free((char *)cfg.path_to_webpage);
    free((char *)cfg.path_to_css);
    free((char *)cfg.relative_path_to_originals);
    free((char *)cfg.relative_path_to_thumbnails);
}


bool create_user_paths(
        const char *user_path,
        const char *srcs_path,
        const char *thmbs_path)
{
    if (mkdir_p(srcs_path, 0777)) {
        LOG_E("Could not create directory %s: %s", srcs_path, strerror(errno));
        return false;
    }

    LOG_I("Created directory %s", srcs_path);

    if (mkdir_p(thmbs_path, 0777)) {
        LOG_E("Could not create directory %s: %s", thmbs_path, strerror(errno));
        return false;
    }

    LOG_I("Created directory %s", thmbs_path);

    return true;
}


int clean_paths(const WebAlbumParams &cfg)
{
    int ret = 0;
    if (rm_rf(cfg.path_to_unpack)) {
        LOG_E("Could not delete original photos at %s", cfg.path_to_unpack);
        ret = -1;
    }

    if (remove(cfg.path_to_unpack)) {
        LOG_E("Could not delete dir with the original photos at %s", cfg.path_to_unpack);
        ret = -1;
    }

    if (rm_rf(cfg.path_to_thumbnails)) {
        LOG_E("Could not delete thumbnails at %s", cfg.path_to_thumbnails);
        ret = -1;
    }

    if (remove(cfg.path_to_thumbnails)) {
        LOG_E("Could not delete dir with the thumbnails at %s", cfg.path_to_thumbnails);
        ret = -1;
    }

    return ret;
}


