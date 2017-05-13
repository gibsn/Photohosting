#include "cgi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern "C" {
#include "ccgi.h"
}

#include "cfg.h"
#include "exceptions.h"
#include "photohosting.h"


Cgi::Cgi(const Config &cfg, Photohosting *_photohosting)
    : photohosting(_photohosting)
{
}


Cgi::~Cgi()
{
}


bool Cgi::Init()
{
    srand(time(NULL) ^ (getpid() << 16));
    return true;
}


#define METHOD_IS(str)\
    strlen(req_method) == strlen(str) && \
    !strcmp(req_method, str)

void Cgi::Routine()
{
    char *req_method = getenv("REQUEST_METHOD");
    if (!req_method) {
        SetStatus(http_internal_error);
        return;
    }

    if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        SetStatus(http_not_found);
    }
}

#undef METHOD_IS

#define HTTP_RESPONSE_ENTRY(INT, ENUM, STATUS, BODY) \
    case ENUM:                                       \
        fputs("Status: " STATUS "\n", stdout);       \
        break;

void Cgi::SetStatus(http_status_t status)
{
    switch(status) {
        HTTP_RESPONSE_GEN
    }

    CloseHeaders();
}

#undef HTTP_RESPONSE_ENTRY


void Cgi::SetContentType(const char *type)
{
    fputs("Content-type: ", stdout);
    fputs(type, stdout);
    fputs("\n", stdout);

    CloseHeaders();
}


void Cgi::SetCookie(const char *key, const char *value)
{
    fputs("Set-cookie: ", stdout);
    fputs(key, stdout);
    fputs("=", stdout);
    fputs(value, stdout);
    fputs("\n", stdout);

    CloseHeaders();
}


void Cgi::SetLocation(const char *location)
{
    fputs("Location: ", stdout);
    fputs(location, stdout);
    fputs("\n", stdout);
}


void Cgi::CloseHeaders()
{
    fputs("\n", stdout);
}


void Cgi::Respond(const char *text)
{
    fputs(text, stdout);
}


// All get requests are supposed to be processed by the HTTP server, not cgi
void Cgi::ProcessGetRequest()
{
    SetStatus(http_internal_error);
}


#define QUERY_IS(str)\
    strlen(query) == strlen(str) && \
    !strcmp(query, str)

void Cgi::ProcessPostRequest()
{
    CGI_varlist *query_list = CGI_get_query(NULL);

    CGI_value query = CGI_lookup(query_list, "q");
    if (!query) {
        SetStatus(http_not_found);
        goto fin;
    }

    if (QUERY_IS("upload_photos")) {
        ProcessUploadPhotos();
    } else if (QUERY_IS("login")) {
        ProcessLogin();
    } else if (QUERY_IS("logout")) {
        ProcessLogout();
    } else {
        SetStatus(http_not_found);
    }

fin:
    CGI_free_varlist(query_list);
}

#undef QUERY_IS


// TODO
void Cgi::ProcessUploadPhotos()
{
    char *user = NULL;
    CGI_varlist *multipart = NULL;
    CGI_varlist *cookies = CGI_get_cookie(NULL);
    if (!cookies) {
        SetStatus(http_forbidden);
        Respond("You are not signed in");
        return;
    }

    CGI_value sid = CGI_lookup(cookies, "sid");
    if (!sid) {
        SetStatus(http_forbidden);
        Respond("You are not signed in");
        goto fin;
    }

    try {
        user = photohosting->GetUserBySession(sid);
        if (!user) {
            SetStatus(http_forbidden);
            Respond("You are not signed in");
            goto fin;
        }
    } catch (GetUserBySessionEx &) {
        SetStatus(http_internal_error);
        goto fin;
    }

    try {
        const char *path_to_tmp_files = photohosting->GetPathToTmpFiles();
        char *tmp_file_path = (char *)malloc(strlen(path_to_tmp_files) + 1 + sizeof "photosXXXXXX");
        strcpy(tmp_file_path, path_to_tmp_files);
        strcat(tmp_file_path, "/");
        strcat(tmp_file_path, "photosXXXXXX");

        multipart = CGI_get_post(NULL, tmp_file_path);
        free(tmp_file_path);
        if (!multipart) {
            SetStatus(http_bad_request);
            goto fin;
        }

        CGI_value archive = CGI_lookup(multipart, "file");
        if (!archive) {
            SetStatus(http_bad_request);
            goto fin;
        }

        // TODO: get page title from somewhere
        char *album_path = photohosting->CreateAlbum(user, archive, "test_album");
        if (album_path) {
            SetLocation(album_path);
            SetStatus(http_see_other);
            free(album_path);
        } else {
            SetStatus(http_bad_request);
            Respond("Bad data");
        }
        //TODO now these 2 exs are not being thrown
    } catch (NoSpace &) {
        SetStatus(http_insufficient_storage);
    } catch (SystemEx &) {
        SetStatus(http_internal_error);
    } catch (PhotohostingEx &) {
        SetStatus(http_internal_error);
    }

fin:
    if (user) free(user);
    CGI_free_varlist(cookies);
    CGI_free_varlist(multipart);
}


void Cgi::ProcessLogin()
{
    CGI_varlist *auth_data = CGI_get_post(NULL, NULL);
    if (!auth_data) {
        SetStatus(http_bad_request);
        Respond("Login and password have not been provided");
        return;
    }

    CGI_value user = CGI_lookup(auth_data, "login");
    CGI_value password = CGI_lookup(auth_data, "password");
    if (!user || !password) {
        SetStatus(http_bad_request);
        Respond("Missing login/password");
        goto fin;
    }

    try {
        char *new_sid = photohosting->Authorise(user, password);
        if (!new_sid) {
            SetStatus(http_bad_request);
            Respond("Incorrect login/password");
            goto fin;
        }

        SetCookie("sid", new_sid);
        Respond("You have been successfully authorized");

        free(new_sid);
    } catch (NewSessionEx &) {
        SetStatus(http_internal_error);
    }

fin:
    CGI_free_varlist(auth_data);
}

void Cgi::ProcessLogout()
{
    CGI_varlist *cookies = CGI_get_cookie(NULL);
    if (!cookies) {
        SetStatus(http_bad_request);
        Respond("You are not signed in");
        return;
    }

    CGI_value sid = CGI_lookup(cookies, "sid");
    if (!sid) {
        SetStatus(http_bad_request);
        Respond("You are not signed in");
        goto fin;
    }

    try {
        char *user = photohosting->GetUserBySession(sid);
        if (!user) {
            SetStatus(http_bad_request);
            Respond("You are not signed in");
            goto fin;
        }

        free(user);

        photohosting->Logout(sid);

        SetCookie("sid", "");
        Respond("You have successfully logged out");
    } catch (SystemEx &) {
        SetStatus(http_internal_error);
    }

fin:
    CGI_free_varlist(cookies);
}
