#include "http_session.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "picohttpparser.h"

#include "auth.h"
#include "common.h"
#include "exceptions.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "tcp_session.h"
#include "log.h"
#include "multipart.h"
#include "photohosting.h"


HttpSession::HttpSession(TcpSession *_tcp_session, HttpServer *_http_server)
    : http_server(_http_server),
    photohosting(_http_server->GetPhotohosting()),
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

    delete read_buf;
    free(s_addr);

    delete request;
    delete response;
}


void HttpSession::Close()
{
    LOG_I("http: closing session");
    active = false;
}


void HttpSession::InitHttpResponse(http_status_t status)
{
    response = new HttpResponse(status, request->minor_version, keep_alive);
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
    ByteArray *tcp_read_buf = tcp_session->GetReadBuf();

    if (!read_buf) read_buf = new ByteArray;
    read_buf->Append(tcp_read_buf);
    delete tcp_read_buf;

    // hexdump((uint8_t *)read_buf->data, read_buf->size);

    switch(ParseHttpRequest(read_buf)) {
    case ok:
        break;
    case incomplete_request:
        return true;
    case invalid_request:
        return false;
    }

    if (!ValidateLocation(strndup(request->path, request->path_len))) {
        InitHttpResponse(http_bad_request);
    } else if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        InitHttpResponse(http_not_found);
    }

    Respond();

    if (!keep_alive) tcp_session->SetWantToClose(true);
    PrepareForNextRequest();

    return true;
}

#undef LOCATION_CONTAINS
#undef METHOD_IS


#define LOCATION_IS(str) !strcmp(path, str)
#define LOCATION_STARTS_WITH(str) path == strstr(path, str)

void HttpSession::ProcessStatic(const char *path)
{
    struct stat _stat;
    if (-1 == http_server->GetFileStat(path, &_stat)) {
        InitHttpResponse(http_not_found);
        return;
    }

    if (request->if_modified_since) {
        struct tm if_modified_since_tm;

        if (!strptime(
                request->if_modified_since,
                "%a, %d %b %Y %H:%M:%S GMT",
                &if_modified_since_tm
        )){
            InitHttpResponse(http_bad_request);
            return;
        }

        if (difftime(_stat.st_mtime, timegm(&if_modified_since_tm)) <= 0) {
            InitHttpResponse(http_not_modified);
            response->AddLastModifiedHeader(_stat.st_mtime);
            return;
        }
    }

    ByteArray *file = http_server->GetFileByPath(path);
    if (!file) {
        InitHttpResponse(http_internal_error);
        return;
    }

    InitHttpResponse(http_ok);
    response->AddLastModifiedHeader(_stat.st_mtime);
    response->SetBody(file);

    delete file;
}


void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/")) {
        InitHttpResponse(http_see_other);
        response->AddLocationHeader("/static/index.html");
    } else if (LOCATION_STARTS_WITH("/static/")) {
        ProcessStatic(path);
    } else {
        InitHttpResponse(http_not_found);
    }

    free(path);
}


void HttpSession::ProcessPhotosUpload()
{
    char *user = NULL;
    char *archive_path = NULL;
    char *page_title = NULL;
    char *album_path = NULL;

    LOG_I("http: client from %s is trying to upload photos", s_addr);

    try {
        user = photohosting->GetUserBySession(request->sid);
        if (!user) {
            LOG_I("http: client from %s is not authorised, responding 403", s_addr);
            InitHttpResponse(http_forbidden);
            goto fin;
        }

        archive_path = UploadFile(user);

        LOG_I("http: creating new album for user \'%s\'", user);
        album_path = photohosting->CreateAlbum(user, archive_path, "test_album");

        LOG_I("http: the album for user \'%s\' has been successfully created at %s",
            user, album_path);

        InitHttpResponse(http_see_other);
        response->AddLocationHeader(album_path);
    } catch (NoSpace &ex) {
        LOG_E("http: responding 507 to %s", user);
        InitHttpResponse(http_insufficient_storage);
    } catch (UserEx &ex) {
        LOG_I("http: responding 400 to %s", user);
        InitHttpResponse(http_bad_request);
        response->SetBody("Bad data");
    } catch (...) {
        LOG_E("http: responding 500 to %s", user);
        InitHttpResponse(http_internal_error);
    }

fin:
    free(user);
    free(archive_path);
    free(page_title);
    free(album_path);
}


void HttpSession::ProcessLogin()
{
    char *user = Auth::ParseLoginFromReq(request->body, request->body_len);
    char *password = Auth::ParsePasswordFromReq(request->body, request->body_len);
    if (!user || !password) {
        InitHttpResponse(http_bad_request);
        goto fin;
    }

    try {
        char *new_sid = photohosting->Authorise(user, password);
        if (!new_sid) {
            InitHttpResponse(http_bad_request);
            LOG_I("http: client from %s failed to authorise as user %s", s_addr, user);
            goto fin;
        }

        LOG_I("http: user %s has authorised from %s", user, s_addr);

        InitHttpResponse(http_ok);
        response->AddCookieHeader("sid", new_sid);

        free(new_sid);
    } catch (AuthEx &ex) {
        InitHttpResponse(http_internal_error);
    }

fin:
    free(user);
    free(password);
}


void HttpSession::ProcessLogout()
{
    char *user = NULL;

    try {
        user = photohosting->GetUserBySession(request->sid);
        if (!user) {
            LOG_I("http: unauthorised user from %s attempted to sign out", s_addr);
            InitHttpResponse(http_bad_request);
            response->SetBody("You are not signed in");
            return;
        }

        photohosting->Logout(request->sid);
        LOG_I("http: user %s has signed out from %s", user, s_addr);

        InitHttpResponse(http_ok);
        response->AddCookieHeader("sid", "");
    } catch (AuthEx &ex) {
        if (user) {
            LOG_E("http: could not log out user %s", user);
        } else {
            LOG_E("http: could not log out client from %s", s_addr);
        }

        InitHttpResponse(http_internal_error);
    }

    free(user);
}


void HttpSession::ProcessPostRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/upload/photos")) {
        ProcessPhotosUpload();
    } else if (LOCATION_IS("/login")) {
        ProcessLogin();
    } else if (LOCATION_IS("/logout")) {
        ProcessLogout();
    } else {
        InitHttpResponse(http_not_found);
    }

    free(path);
}

#undef LOCATION_IS
#undef LOCATION_STARTS_WITH


char *HttpSession::UploadFile(const char *user)
{
    ByteArray* file = NULL;
    try {
        file = GetFileFromRequest();
        if (!file) throw HttpBadFile(user);

        LOG_I("http: got file from user %s (%d bytes)", user, file->size);

        return photohosting->SaveTmpFile(file);
    } catch (...) {
        delete file;
        throw;
    }
}


ByteArray *HttpSession::GetFileFromRequest() const
{
    char *boundary = request->GetMultipartBondary();
    if (!boundary) {
        return NULL;
    }

    MultipartParser parser(boundary);
    parser.Execute(ByteArray(request->body, request->body_len));

    ByteArray *body = parser.GetBody();

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
        LOG_W("http: got an incomplete request")
        ret = incomplete_request;
        goto fin;
    } else if (res == -2) {
        LOG_E("http: failed to parse request");
        ret = invalid_request;
        goto fin;
    }

    request->headers_len = res;
    ProcessHeaders();

    body_bytes_pending = request->body_len - (req->size - res);
    if (body_bytes_pending) {
        LOG_D("http: missing %d body bytes, waiting", body_bytes_pending);
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
        LOG_D("http: %.*s: %.*s",
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
        } else if (CMP_HEADER("If-Modified-Since")) {
            request->if_modified_since = strndup(headers[i].value, headers[i].value_len);
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

