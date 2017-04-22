#include "http_session.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "picohttpparser.h"

#include "auth.h"
#include "common.h"
#include "exceptions.h"
#include "log.h"
#include "multipart.h"


HttpSession::HttpSession(TcpSession *_tcp_session, HttpServer *_http_server)
    : http_server(_http_server),
    s_addr(NULL),
    read_buf(NULL),
    active(true),
    tcp_session(_tcp_session),
    request(NULL),
    response(NULL),
    keep_alive(true)
{
    s_addr = strdup(tcp_session->GetSAddr());
}


HttpSession::~HttpSession()
{
    if (active) this->Close();

    if (read_buf) delete read_buf;
    if (s_addr) free(s_addr);

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
    // hexdump((uint8_t *)read_buf->data, read_buf->size);
    LOG_D("Processing HTTP-request");

    if (!ValidateLocation(strndup(request->path, request->path_len))) {
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
    } else if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
    }

    Respond();

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

void HttpSession::ProcessStatic(const char *path)
{
    ByteArray *file = http_server->GetFileByLocation(path);

    if (!file) {
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
        return;
    }


    response = new HttpResponse(http_ok, request->minor_version, keep_alive);
    response->SetBody(file);

    delete file;
}


void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/")) {
        response = new HttpResponse(http_ok, request->minor_version, keep_alive);
    } else if (LOCATION_STARTS_WITH("/static/")) {
        ProcessStatic(path);
    } else {
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
    }

    free(path);
}


void HttpSession::ProcessPhotosUpload()
{
    LOG_I("Client from %s is trying to upload photos", s_addr);

    char *user = http_server->GetUserBySession(request->sid);
    if (!user) {
        LOG_I("Client from %s is not authorised, responding 403", s_addr);
        response = new HttpResponse(http_forbidden, request->minor_version, keep_alive);
        return;
    }

    try {
        char *album_path = CreateWebAlbum(user, "test_album");

        response = new HttpResponse(http_see_other, request->minor_version, keep_alive);
        response->AddLocationHeader(album_path);
    } catch (NoSpace &) {
        response = new HttpResponse(http_insufficient_storage, request->minor_version, keep_alive);
    } catch (PhotohostingEx &) {
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
    }
}


void HttpSession::ProcessAuth()
{
    char *new_sid = NULL;
    char *user = Auth::ParseLoginFromReq(request->body, request->body_len);
    char *password = Auth::ParsePasswordFromReq(request->body, request->body_len);
    if (!user || !password) {
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
        goto fin;
    }

    new_sid = http_server->Authorise(user, password);
    if (!new_sid) {
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
        LOG_I("Client from %s failed to authorise as user %s", s_addr, user);
        goto fin;
    }

    LOG_I("User %s has authorised from %s", user, s_addr);

    response = new HttpResponse(http_ok, request->minor_version, keep_alive);
    response->SetCookie("sid", new_sid);

fin:
    if (user) free(user);
    if (password) free(password);
    if (new_sid) free(new_sid);
}


void HttpSession::ProcessLogout()
{
    char *user = http_server->GetUserBySession(request->sid);

    if (http_server->Logout(request->sid)) {
        LOG_I("User %s has signed out from %s", user, s_addr);
        response = new HttpResponse(http_ok, request->minor_version, keep_alive);
        response->SetCookie("sid", "");
    } else {
        LOG_E("Could not log out user %s", user);
        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
    }

    free(user);
}


void HttpSession::ProcessPostRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/upload/photos")) {
        ProcessPhotosUpload();
    } else if (LOCATION_IS("/auth")) {
        ProcessAuth();
    } else if (LOCATION_IS("/logout")) {
        ProcessLogout();
    } else {
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
    }

    free(path);
}

#undef LOCATION_IS
#undef LOCATION_STARTS_WITH


char *HttpSession::CreateWebAlbum(const char *user, const char *page_title)
{
    char *file_path = NULL;
    char *album_path = NULL;

    try {
        file_path = UploadFile(user);

        // TODO: get page title from somewhere
        LOG_I("Creating new album for user \'%s\'", user);
        album_path = http_server->CreateAlbum(user, file_path, page_title);

        LOG_I("The album for user \'%s\' has been successfully created at %s", user, album_path);
    } catch (PhotohostingEx &) {
        if (file_path) free(file_path);
        throw;
    }

    return album_path;
}


char *HttpSession::UploadFile(const char *user)
{
    char *name = NULL;
    ByteArray* file = NULL;
    char *saved_file_path = NULL;

    try {
        // TODO: not quite right (can be more than one file)
        file = GetFileFromRequest(&name);
        if (!file || !name) throw HttpBadFile(user);

        LOG_I("Got file \'%s\' from user %s (%d bytes)", name, user, file->size);
        saved_file_path = http_server->SaveFile(file, name);
    } catch (PhotohostingEx &ex) {
        LOG_E("%s\n", ex.GetErrMsg());

        if (name) free(name);
        if (file) delete file;

        throw;
    }

    return saved_file_path;
}


ByteArray *HttpSession::GetFileFromRequest(char **name) const
{
    char *boundary = request->GetMultipartBondary();
    if (!boundary) return NULL;

    MultipartParser parser(boundary);
    parser.Execute(ByteArray(request->body, request->body_len));

    ByteArray *body = parser.GetBody();
    char *_name = parser.GetFilename();

    if (_name && strlen(_name) > 0) {
        *name = strdup(_name);
    }

    free(boundary);

    return body;
}


void HttpSession::Respond()
{
    if (!response->body) {
        response->AddDefaultBodies();
    }

    ByteArray *r = response->GetResponseByteArray();
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
        } else if (CMP_HEADER("Cookie")) {
            request->ParseCookie(headers[i].value, headers[i].value_len);
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

