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

    if (read_buf) delete read_buf;
    if (s_addr) free(s_addr);

    if (request) delete request;
    if (response) delete response;
}


void HttpSession::Close()
{
    LOG_I("Closing HTTP-session");
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
    struct stat _stat;
    if (-1 == http_server->GetFileStat(path, &_stat)) {
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
        return;
    }

    if (request->if_modified_since) {
        struct tm if_modified_since_tm;

        if (!strptime(
                request->if_modified_since,
                "%a, %d %b %Y %H:%M:%S GMT",
                &if_modified_since_tm
        )){
            response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
            return;
        }

        if (difftime(_stat.st_mtime, timegm(&if_modified_since_tm)) <= 0) {
            response = new HttpResponse(http_not_modified, request->minor_version, keep_alive);
            response->AddLastModifiedHeader(_stat.st_mtime);
            return;
        }
    }

    ByteArray *file = http_server->GetFileByPath(path);
    if (!file) {
        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
        return;
    }

    response = new HttpResponse(http_ok, request->minor_version, keep_alive);
    response->AddLastModifiedHeader(_stat.st_mtime);
    response->SetBody(file);

    delete file;
}


void HttpSession::ProcessGetRequest()
{
    char *path = strndup(request->path, request->path_len);

    if (LOCATION_IS("/")) {
        response = new HttpResponse(http_see_other, request->minor_version, keep_alive);
        response->AddLocationHeader("/static/index.html");
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
    char *user = NULL;

    try {
        user = photohosting->GetUserBySession(request->sid);
    } catch (AuthEx &ex) {
        LOG_E("%s", ex.GetErrMsg());
        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
        return;
    }

    if (!user) {
        LOG_I("Client from %s is not authorised, responding 403", s_addr);
        response = new HttpResponse(http_forbidden, request->minor_version, keep_alive);
        return;
    }

    try {
        // TODO: get page title from somewhere
        char *album_path = CreateWebAlbum(user, "test_album");
        if (album_path) {
            response = new HttpResponse(http_see_other, request->minor_version, keep_alive);
            response->AddLocationHeader(album_path);
            free(album_path);
        } else {
            response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
            response->SetBody("Bad data");
        }
    } catch (NoSpace &ex) {
        LOG_E("Responding 507 to %s", user);
        response = new HttpResponse(http_insufficient_storage, request->minor_version, keep_alive);
    } catch (UserEx &ex) {
        LOG_I("Responding 404 to %s", user);
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
        response->SetBody(ex.GetErrMsg());
    } catch (Exception &ex) {
        LOG_E("Responding 500 to %s", user);
        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
    }

    free(user);
}


void HttpSession::ProcessLogin()
{
    char *user = Auth::ParseLoginFromReq(request->body, request->body_len);
    char *password = Auth::ParsePasswordFromReq(request->body, request->body_len);
    if (!user || !password) {
        response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
        goto fin;
    }

    try {
        char *new_sid = photohosting->Authorise(user, password);
        if (!new_sid) {
            response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
            LOG_I("Client from %s failed to authorise as user %s", s_addr, user);
            goto fin;
        }

        LOG_I("User %s has authorised from %s", user, s_addr);

        response = new HttpResponse(http_ok, request->minor_version, keep_alive);
        response->AddCookieHeader("sid", new_sid);

        free(new_sid);
    } catch (AuthEx &ex) {
        LOG_E("%s", ex.GetErrMsg());
        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
        goto fin;
    }

fin:
    if (user) free(user);
    if (password) free(password);
}


void HttpSession::ProcessLogout()
{
    char *user = NULL;

    try {
        user = photohosting->GetUserBySession(request->sid);

        if (!user) {
            LOG_W("Unauthorised user from %s attempted to sign out", s_addr);
            response = new HttpResponse(http_bad_request, request->minor_version, keep_alive);
            response->SetBody("You are not signed in");
            goto fin;
        }

        photohosting->Logout(request->sid);
        LOG_I("User %s has signed out from %s", user, s_addr);

        response = new HttpResponse(http_ok, request->minor_version, keep_alive);
        response->AddCookieHeader("sid", "");
    } catch (AuthEx &ex) {
        if (user) {
            LOG_E("Could not log out user %s (%s)", user, ex.GetErrMsg());
        } else {
            LOG_E("Could not log out client from %s (%s)", s_addr, ex.GetErrMsg());
        }

        response = new HttpResponse(http_internal_error, request->minor_version, keep_alive);
    }

fin:
    if (user) free(user);
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
        response = new HttpResponse(http_not_found, request->minor_version, keep_alive);
    }

    free(path);
}

#undef LOCATION_IS
#undef LOCATION_STARTS_WITH


char *HttpSession::CreateWebAlbum(const char *user, const char *page_title)
{
    char *archive_path = NULL;

    try {
        archive_path = UploadFile(user);

        LOG_I("Creating new album for user \'%s\'", user);
        char *album_path = photohosting->CreateAlbum(user, archive_path, page_title);
        free(archive_path);

        LOG_I("The album for user \'%s\' has been successfully created at %s", user, album_path);

        return album_path;
    } catch (Exception &) {
        free(archive_path);
        throw;
    }
}


char *HttpSession::UploadFile(const char *user)
{
    char *name = NULL;
    ByteArray* file = NULL;
    char *saved_file_path = NULL;

    try {
        // TODO: not quite right (can be more than one file)
        file = GetFileFromRequest();
        if (!file) throw HttpBadFile(user);

        LOG_I("Got file from user %s (%d bytes)", user, file->size);
        saved_file_path = photohosting->SaveTmpFile(file);

        return saved_file_path;
    } catch (PhotohostingEx &ex) {
        LOG_E("%s\n", ex.GetErrMsg());

        free(name);
        if (file) delete file;

        throw;
    }
}


ByteArray *HttpSession::GetFileFromRequest() const
{
    char *boundary = request->GetMultipartBondary();
    if (!boundary) return NULL;

    MultipartParser parser(boundary);
    parser.Execute(ByteArray(request->body, request->body_len));

    ByteArray *body = parser.GetBody();
    char *_name = parser.GetFilename();

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

