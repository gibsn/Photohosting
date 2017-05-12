#include "cgi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "ccgi.h"
}

#include "cfg.h"
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
        // response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
    }
}

#undef METHOD_IS


// All get requests are supposed to be processed by the HTTP server, not cgi
void Cgi::ProcessGetRequest()
{
    // Respond(http_internal_error);
}


#define QUERY_IS(str)\
    strlen(query) == strlen(str) && \
    !strcmp(query, str)

void Cgi::ProcessPostRequest()
{
    CGI_varlist *query_list = CGI_get_query(NULL);

    CGI_value query = CGI_lookup(query_list, "q");
    // if (!query) Respond(http_bad_request);

    if (QUERY_IS("upload_photos")) {
        ProcessUploadPhotos();
    } else if (QUERY_IS("login")) {
        ProcessLogin();
    } else if (QUERY_IS("logout")) {
        ProcessLogout();
    } else {
        // Respond(http_bad_request);
    }
}

#undef QUERY_IS


void Cgi::ProcessUploadPhotos()
{
}

void Cgi::ProcessLogin()
{
}

void Cgi::ProcessLogout()
{
}
