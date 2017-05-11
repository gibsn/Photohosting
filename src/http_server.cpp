#include "http_server.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "WebAlbumCreator.h"

#include "auth.h"
#include "cfg.h"
#include "common.h"
#include "exceptions.h"
#include "http_request.h"
#include "http_response.h"
#include "http_session.h"
#include "log.h"
#include "photohosting.h"
#include "web_album_creator_helper.h"


char *HttpServer::AddPathToStaticPrefix(const char *path) const
{
    int file_path_len = strlen(path) - (sizeof "/static" - 1) + 1;
    file_path_len += path_to_static_len;

    char *file_path = (char *)malloc(file_path_len);
    if (!file_path) return NULL;

    strcpy(file_path, path_to_static);
    strcat(file_path, path + sizeof "/static" - 1);

    return file_path;
}


//TODO: not sure if it is good to return TcpSession * here
TcpSession *HttpServer::CreateNewSession()
{
    TcpSession *tcp_session = TcpServer::CreateNewSession();
    // TODO: here TcpServer returns NULL when EAGAIN happens as well
    if (!tcp_session) {
        LOG_D("Could not open a new HTTP-session");
        return NULL;
    }

    HttpSession *http_session = new HttpSession(tcp_session, this);

    tcp_session->SetSessionDriver(http_session);

    return tcp_session;
}


// TODO: should differentiate situations when file doesnt exist and is forbidden
ByteArray *HttpServer::GetFileByLocation(const char *path)
{
    char *file_path = AddPathToStaticPrefix(path);

    ByteArray *file = read_file(file_path);
    free(file_path);

    return file;
}


char *HttpServer::SaveFile(ByteArray *file, char *name)
{
    //TODO: name can be too long
    char *full_path = (char *)malloc(strlen(path_to_tmp_files) + 2 + strlen(name));
    strcpy(full_path, path_to_tmp_files);
    strcat(full_path, "/");
    strcat(full_path, name);

    int fd = open(full_path, O_CREAT | O_WRONLY, 0666);
    try {
        if (fd == -1) {
            LOG_E("Could not save file %s: %s", full_path, strerror(errno));
            throw SaveFileEx();
        }

        int n = write(fd, file->data, file->size);
        if (file->size != n) {
            LOG_E("Could not save file %s: %s", full_path, strerror(errno));
            if (errno == ENOSPC) throw NoSpace();
            throw SaveFileEx();
        }

        return full_path;
    }
    catch (PhotohostingEx &) {
        if (full_path) free(full_path);
        throw;
    }
}


char *HttpServer::Authorise(const char *user, const char *password)
{
    if (auth->Check(user, password)) {
        return auth->NewSession(user);
    }

    return NULL;
}


void HttpServer::Logout(const char *sid)
{
    if (*sid == '\0') return;

    return auth->DeleteSession(sid);
}


// throws GetUserBySessionEx
char *HttpServer::GetUserBySession(const char *sid) {
    if (!sid || *sid == '\0') return NULL;

    return auth->GetUserBySession(sid);
}


HttpServer::HttpServer(const Config &cfg, Photohosting *_photohosting, AuthDriver *_auth)
    : TcpServer(cfg),
    path_to_static_len(0),
    photohosting(_photohosting),
    auth(_auth)
{
    path_to_static = strdup(cfg.path_to_static);
    assert(path_to_static);
    path_to_static_len = strlen(path_to_static);

    path_to_tmp_files = strdup(cfg.path_to_tmp_files);
    assert(path_to_tmp_files);

    path_to_css = cfg.path_to_css;
    assert(path_to_css);
}


HttpServer::~HttpServer()
{
    if (path_to_static) free(path_to_static);
    if (path_to_tmp_files) free(path_to_tmp_files);
    if (path_to_css) free(path_to_css);
}

