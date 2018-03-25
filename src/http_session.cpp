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
    : state(state_idle),
    should_wait_for_more_data(true),
    http_server(_http_server),
    photohosting(_http_server->GetPhotohosting()),
    last_headers_len(0),
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

    response.minor_version = 1;
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


void HttpSession::OnRead()
{
    ByteArray *tcp_read_buf = tcp_session->GetReadBuf();
    read_buf.Append(tcp_read_buf);
    delete tcp_read_buf;

    should_wait_for_more_data = false;

    if (log_level >= LOG_DEBUG) {
        hexdump((uint8_t *)read_buf.data, read_buf.size);
    }

    return Routine();
}


void HttpSession::OnWrite()
{
}


#define LOCATION_IS(str) !strcmp(path, str)

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

#define LOCATION_STARTS_WITH(str) path == strstr(path, str)

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

#undef LOCATION_STARTS_WITH


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
    LOG_D("http: responding %d to %s", response.code, s_addr);

    if (!response.body) {
        response.AddDefaultBodies();
    }

    ByteArray *r = response.GetResponseByteArray();
    tcp_session->Send(r);

    delete r;
}
