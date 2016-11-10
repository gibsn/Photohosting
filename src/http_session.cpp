#include "http_session.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "log.h"


HttpSession::HttpSession()
    : fd(0),
    request(NULL),
    response(NULL),
    keep_alive(true)
{
}


HttpSession::HttpSession(int _fd)
    : fd(_fd),
    request(NULL),
    response(NULL),
    keep_alive(true)
{
}


HttpSession::~HttpSession()
{
    if (request) delete request;
    if (response) delete response;
}


char *HttpSession::RespondAny()
{
    response->CloseHeaders();

    response->AddBody();

    response->response = (char *)realloc(response->response, response->response_len + 1);
    response->response[response->response_len] = '\0';

    return response->response;
}


char *HttpSession::RespondOk()
{
    response->minor_version = request->minor_version;
    response->keep_alive = keep_alive;

    response->code = http_ok;

    response->body_len = 0;
    response->body = NULL;

    response->AddDefaultHeaders();

    return RespondAny();
}


char *HttpSession::RespondPermanentRedirect(const char *location)
{
    response->minor_version = request->minor_version;
    response->keep_alive = keep_alive;

    response->code = http_permanent_redirect;


    response->body_len = 0;
    response->body = NULL;

    response->AddDefaultHeaders();

    response->AddHeader("Location", location);

    return RespondAny();
}



char *HttpSession::RespondNotFound()
{
    response->minor_version = request->minor_version;
    response->keep_alive = keep_alive;

    response->code = http_not_found;

    response->body_len = 0;
    response->body = NULL;

    response->AddDefaultHeaders();

    return RespondAny();
}


char *HttpSession::RespondNotImplemented()
{
    response->minor_version = request->minor_version;
    response->keep_alive = keep_alive;

    response->code = http_not_implemented;

    response->body_len = 0;
    response->body = NULL;

    response->AddDefaultHeaders();

    return RespondAny();
}


bool HttpSession::ParseHttpRequest(char *req)
{
    int req_len = strlen(req);
    request = new HttpRequest;
    response = new HttpResponse;

    int res = phr_parse_request(req,
                                req_len,
                                (const char**)&request->method,
                                &request->method_len,
                                (const char **)&request->path,
                                &request->path_len,
                                &request->minor_version,
                                request->headers,
                                &request->n_headers,
                                0);

    if (res == -1) {
        LOG_E("Failed to parse HTTP-Request");
        return false;
    } else if (res == -2) {
        LOG_W("Got an incomplete HTTP-Request, dropping connection")
        return false;
    }

    //strdup here?
    request->body = req + res;
    ProcessHeaders();

    return true;
}


void HttpSession::ProcessHeaders()
{
#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

    struct phr_header *headers = request->headers;
    for (int i = 0; i < request->n_headers; ++i) {
        if (CMP_HEADER("Connection")) {
            keep_alive =
                headers[i].value_len > 0 &&
                tolower(headers[i].value[0]) == 'k';
        } else if (CMP_HEADER("Content-Length")) {
            request->body_len = strtol(headers[i].value, NULL, 10);
        }
    }

#undef CMP_HEADER
}


void HttpSession::PrepareForNextRequest()
{
    if (response) {
        delete response;
        response = NULL;
    }

    if (request) {
        delete request;
        request = NULL;
    }
}
