#include "cgi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    return true;
}


#define METHOD_IS(str)\
    strlen(req_method) == strlen(str) && \
    !strcmp(req_method, str)

void Cgi::Routine()
{
    char *req_method = getenv("REQUEST_METHOD");
    if (!req_method) exit(-1);

    if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        SetStatus(http_not_found);
    }
}

#undef METHOD_IS


void Cgi::SetStatus(http_status_t status)
{
}


void Cgi::SetCookie(const char *key, const char *value)
{
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
    if (!query) SetStatus(http_bad_request);

    if (QUERY_IS("upload_photos")) {
        ProcessUploadPhotos();
    } else if (QUERY_IS("login")) {
        ProcessLogin();
    } else if (QUERY_IS("logout")) {
        ProcessLogout();
    } else {
        SetStatus(http_bad_request);
    }

    CGI_free_varlist(query_list);
}

#undef QUERY_IS


void Cgi::ProcessUploadPhotos()
{
}

void Cgi::ProcessLogin()
{
    fputs("Content-type: text/plain\r\n\r\n", stdout);

    CGI_varlist *auth_data = CGI_get_post(NULL, NULL);
    if (!auth_data) {
        SetStatus(http_bad_request);
        fputs("Login and password have not been provided\n", stdout);
        return;
    }

    CGI_value user = CGI_lookup(auth_data, "login");
    CGI_value password = CGI_lookup(auth_data, "password");
    if (!user || !password) {
        fputs("Missing login/password\n", stdout);
        SetStatus(http_bad_request);
        CGI_free_varlist(auth_data);
        return;
    }

    try {
        char *new_sid = photohosting->Authorise(user, password);
        if (!new_sid) {
            SetStatus(http_bad_request);
            fputs("Incorrect login/password\n", stdout);
            CGI_free_varlist(auth_data);
            return;
        }

        SetCookie("sid", new_sid);

        free(new_sid);
    } catch (NewSessionEx &) {
        SetStatus(http_internal_error);
        CGI_free_varlist(auth_data);
    }
}

void Cgi::ProcessLogout()
{
    fputs("Content-type: text/plain\r\n\r\n", stdout);

    CGI_varlist *cookies = CGI_get_cookie(NULL);
    if (!cookies) {
        SetStatus(http_bad_request);
        fputs("You are not signed in\n", stdout);
        return;
    }

    CGI_value sid = CGI_lookup(cookies, "sid");
    if (!sid) {
        SetStatus(http_bad_request);
        fputs("You are not signed in\n", stdout);
        goto fin;
    }

    try {
        char *user = photohosting->GetUserBySession(sid);
        if (!user) {
            SetStatus(http_bad_request);
            fputs("You are not signed in\n", stdout);
            goto fin;
        }

        free(user);

        photohosting->Logout(sid);
        SetCookie("sid", "");
    } catch (SystemEx &) {
        SetStatus(http_internal_error);
    }

fin:
    CGI_free_varlist(cookies);
}
