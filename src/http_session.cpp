#include "http_session.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "picohttpparser.h"

#include "common.h"
#include "log.h"
#include "multipart.h"


HttpSession::HttpSession(TcpSession *_tcp_session, HttpServer *_http_server)
    : http_server(_http_server),
    read_buf(NULL),
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

    if (read_buf) delete read_buf;

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

    ByteArray *tcp_read_buf = tcp_session->GetReadBuf();
    tcp_session->TruncateReadBuf();

    if (!read_buf) read_buf = new ByteArray;
    //TODO: guess I can allocate Content-Length just once
    read_buf->Append(tcp_read_buf);

    // hexdump((uint8_t *)read_buf->data, read_buf->size);
    switch(ParseHttpRequest(read_buf)) {
    case ok:
        break;
    case incomplete_request:
        goto fin;
        break;
    case invalid_request:
        ret = false;
        goto fin;
        break;
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
    PrepareForNextRequest();

fin:
    delete tcp_read_buf;

    return ret;
}

#undef LOCATION_CONTAINS
#undef METHOD_IS


#define LOCATION_IS(str) !strcmp(path, str)
#define LOCATION_STARTS_WITH(str) path == strstr(path, str)

void HttpSession::RespondStatic(const char *path)
{
    ByteArray *file = http_server->GetFileByLocation(path);

    if (!file) {
        Respond(http_not_found);
        return;
    }

    response->SetBody(file);
    Respond(http_ok);
    delete file;
}


void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/")) {
        Respond(http_ok);
    } else if (LOCATION_STARTS_WITH("/static/")) {
        RespondStatic(path);
    } else {
        Respond(http_not_found);
    }

    free(path);
}


void HttpSession::RespondUpload(const char *path)
{
    // TODO: filename here
    ByteArray* file = GetFileFromRequest(path);
    // TODO: not quite right
    if (!file) {
        Respond(http_bad_request);
        return;
    }
    // http_server->SaveFile(file, name);

    delete file;
}


void HttpSession::ProcessPostRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_STARTS_WITH("/upload/")) {
        RespondUpload(path);
    } else {
        Respond(http_not_found);
    }

    free(path);
}

#undef LOCATION_IS
#undef LOCATION_STARTS_WITH


ByteArray *HttpSession::GetFileFromRequest(const char *req_path) const
{
    char *boundary = request->GetMultipartBondary();
    if (!boundary) return NULL;

    MultipartParser parser(boundary);
    parser.Execute(ByteArray(request->body, request->body_len));

    ByteArray *body = parser.GetBody();

    // hexdump((uint8_t*)body->data, body->size);
    free(boundary);

    return NULL;
}


void HttpSession::Respond(http_status_t code)
{
    ByteArray *r = response->GetResponseByteArray(code);
    tcp_session->Send(r);

    delete r;
}


request_parser_result_t HttpSession::ParseHttpRequest(ByteArray *req)
{
    request_parser_result_t ret = ok;
    int body_bytes_pending;

    request = new HttpRequest();

    int res = phr_parse_request(req->data,
                                req->size,
                                (const char**)&request->method,
                                &request->method_len,
                                (const char **)&request->path,
                                &request->path_len,
                                &request->minor_version,
                                request->headers,
                                &request->n_headers,
                                0);

    if (res == -1) {
        LOG_W("Got an incomplete HTTP-Request")
        ret = incomplete_request;
        goto fin;
    } else if (res == -2) {
        LOG_E("Failed to parse HTTP-Request");
        ret = invalid_request;
        goto fin;
    }

    request->headers_len = res;
    ProcessHeaders();

    body_bytes_pending = request->body_len - (req->size - res);
    if (body_bytes_pending) {
        LOG_D("Missing %d body bytes, waiting", body_bytes_pending);
        ret = incomplete_request;
        goto fin;
    }

    request->body = (char *)malloc(request->body_len);
    memcpy(request->body, req->data + request->headers_len, request->body_len);

fin:
    if (ret != ok) {
        delete request;
        request = NULL;
    }

    return ret;
}


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

void HttpSession::ProcessHeaders()
{
    struct phr_header *headers = request->headers;
    for (int i = 0; i < request->n_headers; ++i) {
        LOG_D("%.*s: %.*s",
                int(headers[i].name_len), headers[i].name,
                int(headers[i].value_len), headers[i].value);

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

    if (read_buf) {
        delete read_buf;
        read_buf = NULL;
    }
}
