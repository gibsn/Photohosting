#include "web_album_creator_preprocessor.h"

#include <string.h>
#include <stdlib.h>


//TODO: too much copy-pasting here

char *make_user_path(char *path_to_static, char *user)
{
    int user_path_len = strlen(path_to_static) + sizeof "/" - 1;
    user_path_len += strlen(user) + sizeof "/";
    char *user_path = (char *)malloc(user_path_len);

    strcpy(user_path, path_to_static);
    strcat(user_path, "/");

    strcat(user_path, user);
    strcat(user_path, "/");

    return user_path;
}


char *make_path_to_unpack(char *user_path, char *random_id)
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


char *make_path_to_thumbs(char *user_path, char *random_id)
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


char *make_path_to_webpage(char *user_path, char *random_id)
{
    int path_to_webpage_len = strlen(user_path) + sizeof "/" - 1;
    path_to_webpage_len += strlen(random_id) + sizeof ".html";
    char *path_to_webpage = (char *)malloc(path_to_webpage_len);

    strcpy(path_to_webpage, user_path);

    strcat(path_to_webpage, random_id);
    strcat(path_to_webpage, ".html");

    return path_to_webpage;
}


char *make_path_to_css(char *path_to_css)
{
    int path_to_css_file_len = sizeof "/static/" - 1;
    path_to_css_file_len += strlen(path_to_css) + sizeof "/blue.css";
    char *path_to_css_file = (char *)malloc(path_to_css_file_len);

    strcpy(path_to_css_file, "/static/");

    strcat(path_to_css_file, path_to_css);
    strcat(path_to_css_file, "/blue.css");

    return path_to_css_file;
}


char *make_r_path_to_srcs(char *user, char *random_id)
{
    int r_path_to_srcs_len = sizeof "/static/" - 1 + strlen(user);
    r_path_to_srcs_len +=  sizeof "/srcs/" - 1 + strlen(random_id) + sizeof "/";
    char *r_path_to_srcs = (char *)malloc(r_path_to_srcs_len);

    strcpy(r_path_to_srcs, "/static/");

    strcat(r_path_to_srcs, user);
    strcat(r_path_to_srcs, "/srcs/");

    strcat(r_path_to_srcs, random_id);
    strcat(r_path_to_srcs, "/");

    return r_path_to_srcs;
}


char *make_r_path_to_thmbs(char *user, char *random_id)
{
    int r_path_to_thmbs_len = sizeof "/static/" - 1 + strlen(user);
    r_path_to_thmbs_len +=  sizeof "/thmbs/" - 1 + strlen(random_id) + sizeof "/";

    char *r_path_to_thmbs = (char *)malloc(r_path_to_thmbs_len);

    strcpy(r_path_to_thmbs, "/static/");

    strcat(r_path_to_thmbs, user);
    strcat(r_path_to_thmbs, "/thmbs/");

    strcat(r_path_to_thmbs, random_id);
    strcat(r_path_to_thmbs, "/");

    return r_path_to_thmbs;
}


char *make_r_path_to_webpage(char *user, char *random_id)
{
    int r_path_to_webpage_len = sizeof "/static/" - 1 + strlen(user);
    r_path_to_webpage_len +=  sizeof "/" - 1 + strlen(random_id) + sizeof ".html";

    char *r_path_to_webpage = (char *)malloc(r_path_to_webpage_len);

    strcpy(r_path_to_webpage, "/static/");

    strcat(r_path_to_webpage, user);
    strcat(r_path_to_webpage, "/");

    strcat(r_path_to_webpage, random_id);
    strcat(r_path_to_webpage, ".html");

    return r_path_to_webpage;
}

