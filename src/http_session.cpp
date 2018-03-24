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
    last_len(0),
    tcp_session(_tcp_session),
    keep_alive(true)
{
    s_addr = strdup(tcp_session->GetSAddr());
}


HttpSession::~HttpSession()
{
    Close();
    free(s_addr);
}


void HttpSession::Close()
{
    LOG_I("http: closing session");
}


void HttpSession::InitHttpResponse(http_status_t status)
{
    response.code = status;
    response.minor_version = request.minor_version;
    response.keep_alive = keep_alive;

    response.AddStatusLine();

    response.AddDateHeader();
    response.AddServerHeader();
    response.AddConnectionHeader(keep_alive);
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
    request.method_len == strlen(str) && \
    !strncmp(request.method, str, request.method_len)

void HttpSession::ProcessRequest()
{
    ByteArray *tcp_read_buf = tcp_session->GetReadBuf();

    read_buf.Append(tcp_read_buf);
    delete tcp_read_buf;

    // hexdump((uint8_t *)read_buf.data, read_buf.size);

    switch(ParseHttpRequest()) {
    case incomplete_request:
        LOG_W("http: got an incomplete request")
        last_len = read_buf.size;
        request.Reset();
        return;
    case invalid_request:
        LOG_E("http: failed to parse request");
        InitHttpResponse(http_bad_request);
        tcp_session->SetWantToClose(true);
        goto fin;
    case ok:
        break;
    }

    if (!ValidateLocation(strndup(request.path, request.path_len))) {
        InitHttpResponse(http_bad_request);
    } else if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        InitHttpResponse(http_not_found);
    }

    if (!keep_alive) {
        tcp_session->SetWantToClose(true);
    }

fin:
    Respond();
    Reset();
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

    if (request.if_modified_since) {
        struct tm if_modified_since_tm;

        if (!strptime(
                request.if_modified_since,
                "%a, %d %b %Y %H:%M:%S GMT",
                &if_modified_since_tm
        )){
            InitHttpResponse(http_bad_request);
            return;
        }

        if (difftime(_stat.st_mtime, timegm(&if_modified_since_tm)) <= 0) {
            InitHttpResponse(http_not_modified);
            response.AddLastModifiedHeader(_stat.st_mtime);
            return;
        }
    }

    ByteArray *file = http_server->GetFileByPath(path);
    if (!file) {
        InitHttpResponse(http_internal_error);
        return;
    }

    InitHttpResponse(http_ok);
    response.AddLastModifiedHeader(_stat.st_mtime);
    response.SetBody(file);

    delete file;
}


void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request.path, request.path_len);

    if (LOCATION_IS("/")) {
        InitHttpResponse(http_see_other);
        response.AddLocationHeader("/static/index.html");
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
        user = photohosting->GetUserBySession(request.sid);
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
        response.AddLocationHeader(album_path);
    } catch (NoSpace &ex) {
        LOG_E("http: responding 507 to %s", user);
        InitHttpResponse(http_insufficient_storage);
    } catch (UserEx &ex) {
        LOG_I("http: responding 400 to %s", user);
        InitHttpResponse(http_bad_request);
        response.SetBody("Bad data");
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
    char *user = Auth::ParseLoginFromReq(request.body, request.body_len);
    char *password = Auth::ParsePasswordFromReq(request.body, request.body_len);
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
        response.AddCookieHeader("sid", new_sid);

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
        user = photohosting->GetUserBySession(request.sid);
        if (!user) {
            LOG_I("http: unauthorised user from %s attempted to sign out", s_addr);
            InitHttpResponse(http_bad_request);
            response.SetBody("You are not signed in");
            return;
        }

        photohosting->Logout(request.sid);
        LOG_I("http: user %s has signed out from %s", user, s_addr);

        InitHttpResponse(http_ok);
        response.AddCookieHeader("sid", "");
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
    char *path = strndup(request.path, request.path_len);

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
    char *boundary = request.GetMultipartBondary();
    if (!boundary) {
        return NULL;
    }

    MultipartParser parser(boundary);
    parser.Execute(ByteArray(request.body, request.body_len));

    ByteArray *body = parser.GetBody();

    free(boundary);

    return body;
}


void HttpSession::Respond()
{
    if (!response.body) {
        response.AddDefaultBodies();
    }

    ByteArray *r = response.GetResponseByteArray();
    tcp_session->Send(r);

    delete r;
}


request_parser_result_t HttpSession::ParseHttpRequest()
{
    int res = phr_parse_request(
        read_buf.data,
        read_buf.size,
        (const char**)&request.method,
        &request.method_len,
        (const char **)&request.path,
        &request.path_len,
        &request.minor_version,
        request.headers,
        &request.n_headers,
        last_len
    );

    if (res == -1) {
        return invalid_request;
    } else if (res == -2) {
        return incomplete_request;
    }

    request.headers_len = res;
    ProcessHeaders();

    int body_bytes_pending = request.body_len - (read_buf.size - res);
    if (body_bytes_pending) {
        LOG_D("http: missing %d body bytes, waiting", body_bytes_pending);
        return incomplete_request;
    }

    // TODO no need to allocate and copy
    request.body = (char *)malloc(request.body_len);
    memcpy(request.body, read_buf.data + request.headers_len, request.body_len);

    return ok;
}


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

void HttpSession::ProcessHeaders()
{
    struct phr_header *headers = request.headers;
    for (int i = 0; i < request.n_headers; ++i) {
        LOG_D("http: %.*s: %.*s",
                int(headers[i].name_len), headers[i].name,
                int(headers[i].value_len), headers[i].value);

        if (CMP_HEADER("Connection")) {
            keep_alive =
                headers[i].value_len > 0 &&
                tolower(headers[i].value[0]) == 'k';
        } else if (CMP_HEADER("Content-Length")) {
            request.body_len = strtol(headers[i].value, NULL, 10);
        } else if (CMP_HEADER("Cookie")) {
            request.ParseCookie(headers[i].value, headers[i].value_len);
        } else if (CMP_HEADER("If-Modified-Since")) {
            request.if_modified_since = strndup(headers[i].value, headers[i].value_len);
        }
    }
}

#undef CMP_HEADER


void HttpSession::Reset()
{
    response.Reset();
    request.Reset();
    read_buf.Reset();
}

