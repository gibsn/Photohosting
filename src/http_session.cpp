#include "http_session.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "picohttpparser.h"

#include "common.h"
#include "log.h"


HttpSession::HttpSession(TcpSession *_tcp_session, HttpServer *_http_server)
    : http_server(_http_server),
    active(true),
    tcp_session(_tcp_session),
    request(NULL),
    response(NULL),
    keep_alive(true)
{
}


HttpSession::~HttpSession()
{
    if (active) this->Close();

    if (request) delete request;
    if (response) delete response;
}


void HttpSession::Close()
{
    LOG_I("Closing Http-session");
    active = false;
}


bool HttpSession::ValidateLocation(char *path)
{
    bool ret = false;

    if (strstr(path, "/../")) goto fin;

    ret = true;

fin:
    free(path);
    return ret;
}


#define METHOD_IS(str)\
    request->method_len == strlen(str) && \
    !strncmp(request->method, str, request->method_len)

bool HttpSession::ProcessRequest()
{
    bool ret = true;

    char *read_buf = tcp_session->GetReadBuf();
    tcp_session->TruncateReadBuf();
    // hexdump((uint8_t *)read_buf, strlen(read_buf));
    if (!ParseHttpRequest(read_buf)) {
        ret = false;
        goto fin;
    }

    response = new HttpResponse(http_ok, NULL, request->minor_version, keep_alive);

    LOG_D("Processing HTTP-request");

    if (!ValidateLocation(strndup(request->path, request->path_len))) {
        Respond(http_bad_request);
    } else if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        Respond(http_not_found);
    }

    if (!keep_alive) tcp_session->SetWantToClose(true);

fin:
    PrepareForNextRequest();
    free(read_buf);
    return ret;
}

#undef LOCATION_CONTAINS
#undef METHOD_IS


#define LOCATION_IS(str) !strcmp(path, str)
#define LOCATION_STARTS_WITH(str) path == strstr(path, str)

void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/")) {
        Respond(http_ok);
    } else if (LOCATION_STARTS_WITH("/static/")) {
        ByteArray *file = http_server->GetFileByLocation(path);

        if (file) {
            response->SetBody(file);
            Respond(http_ok);
        } else {
            Respond(http_not_found);
        }

        delete file;
    } else {
        Respond(http_not_found);
    }

    free(path);
}


void HttpSession::ProcessPostRequest()
{
    Respond(http_continue);
}

#undef LOCATION_IS
#undef LOCATION_STARTS_WITH


void HttpSession::Respond(http_status_t code)
{
    ByteArray *r = response->GetResponseByteArray(code);
    tcp_session->Send(r);

    delete r;
}


bool HttpSession::ParseHttpRequest(char *req)
{
    int req_len = strlen(req);
    request = new HttpRequest;

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


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

void HttpSession::ProcessHeaders()
{
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
}

#undef CMP_HEADER


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
