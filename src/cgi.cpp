#include "cgi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "auth.h"
#include "cfg.h"
#include "exceptions.h"
#include "log.h"
#include "photohosting.h"


Cgi::Cgi(const Config &cfg, Photohosting *_photohosting)
    : photohosting(_photohosting)
{
    query_list = CGI_get_query(NULL);
    cookies = CGI_get_cookie(NULL);

    char *tmp_file_path = photohosting->GenerateTmpFilePathTemplate();
    post_body = CGI_get_post(NULL, tmp_file_path);
    free(tmp_file_path);
}


Cgi::~Cgi()
{
    CGI_free_varlist(query_list);
    CGI_free_varlist(cookies);
    CGI_free_varlist(post_body);
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

    if (!query_list) {
        SetStatus(http_bad_request);
        return;
    }

    CGI_value query = CGI_lookup(query_list, "q");
    if (!query) {
        SetStatus(http_not_found);
        return;
    }

    if (METHOD_IS("GET")) {
        ProcessGetRequest(query);
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest(query);
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
    fputs("\n", stdout);
}


#define QUERY_IS(str)\
    strlen(query) == strlen(str) && \
    !strcmp(query, str)

void Cgi::ProcessGetRequest(CGI_value query)
{
    if (QUERY_IS("users")) {
        ProcessUsers();
    } else {
        SetStatus(http_not_found);
    }
}


void Cgi::ProcessUsers()
{
    try {
        Users users_list = photohosting->GetUsers();
        SetStatus(http_ok);

        Users::username_node_t *p = users_list.root;
        while(p) {
            Respond(p->user->GetName());

            Users::username_node_t *next = p->next;
            delete p;

            p = next;
        }
    } catch (const GetUsersEx &) {
        SetStatus(http_internal_error);
    }
}


void Cgi::ProcessPostRequest(CGI_value query)
{
    if (QUERY_IS("upload_photos")) {
        ProcessUploadPhotos();
    } else if (QUERY_IS("login")) {
        ProcessLogin();
    } else if (QUERY_IS("logout")) {
        ProcessLogout();
    } else {
        SetStatus(http_not_found);
    }
}

#undef QUERY_IS


const char *Cgi::GetSidFromCookies()
{
    if (!cookies) {
        return NULL;
    }

    return CGI_lookup(cookies, "sid");
}


char *Cgi::CreateWebAlbum(const char *user)
{
    if (!post_body) {
        LOG_W("cgi: got bad POST-body from user %s", user);
        throw HttpBadPostBody(user);
    }

    CGI_value archive_path = CGI_lookup(post_body, "file");
    if (!archive_path) {
        LOG_W("cgi: got bad file from user %s", user);
        throw HttpBadFile(user);
    }

    CGI_value page_title = CGI_lookup(post_body, "title");
    if (!page_title) {
        LOG_W("cgi: got bad page title from user %s", user);
        throw HttpBadPageTitle(user);
    }

    return photohosting->CreateAlbum(user, archive_path, page_title);
}

void Cgi::ProcessUploadPhotos()
{
    try {
        const char *sid = GetSidFromCookies();
        if (!sid) {
            SetStatus(http_forbidden);
            Respond("You are not signed in");
            return;
        }

        user = photohosting->GetUserBySession(sid);
        if (!user) {
            SetStatus(http_forbidden);
            Respond("You are not signed in");
            return;
        }

        album_path = CreateWebAlbum(user);

        SetLocation(album_path);
        SetStatus(http_see_other);
    } catch (const NoSpace &) {
        SetStatus(http_insufficient_storage);
    } catch (const UserEx &) {
        SetStatus(http_bad_request);
        Respond("Bad data");
    } catch (const AuthEx &) {
        SetStatus(http_internal_error);
    } catch (const PhotohostingEx &) {
        SetStatus(http_internal_error);
    }
}


void Cgi::ProcessLogin()
{
    if (!post_body) {
        SetStatus(http_bad_request);
        Respond("Login and password have not been provided");
        return;
    }

    CGI_value user = CGI_lookup(post_body, "login");
    CGI_value password = CGI_lookup(post_body, "password");
    if (!user || !password) {
        SetStatus(http_bad_request);
        Respond("Missing login/password");
        return;
    }

    try {
        char *new_sid = photohosting->Authorise(user, password);
        if (!new_sid) {
            SetStatus(http_bad_request);
            Respond("Incorrect login/password");
            return;
        }

        SetCookie("sid", new_sid);
        free(new_sid);

        CloseHeaders();
        Respond("You have been successfully authorized");
    } catch (const AuthEx &) {
        SetStatus(http_internal_error);
    }
}

void Cgi::ProcessLogout()
{
    if (!cookies) {
        SetStatus(http_bad_request);
        Respond("You are not signed in");
        return;
    }

    CGI_value sid = CGI_lookup(cookies, "sid");
    if (!sid) {
        SetStatus(http_bad_request);
        Respond("You are not signed in");
        return;
    }

    try {
        char *user = photohosting->GetUserBySession(sid);
        if (!user) {
            SetStatus(http_bad_request);
            Respond("You are not signed in");
            return;
        }

        free(user);
        photohosting->Logout(sid);

        SetCookie("sid", "");
        CloseHeaders();
        Respond("You have successfully logged out");
    } catch (AuthEx &ex) {
        SetStatus(http_internal_error);
    }
}
