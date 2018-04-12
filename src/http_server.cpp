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

#include "cfg.h"
#include "common.h"
#include "exceptions.h"
#include "http_request.h"
#include "http_response.h"
#include "http_session.h"
#include "log.h"
#include "photohosting.h"
#include "tcp_session.h"
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


AppLayerDriver *HttpServer::CreateSession(TcpSession *tcp_session)
{
    assert(tcp_session);

    HttpSession *http_session = new HttpSession(tcp_session, this);
    return http_session;
}


void HttpServer::CloseSession(AppLayerDriver *session)
{
    HttpSession *http_session = dynamic_cast<HttpSession *>(session);
    delete http_session;
}


// TODO: should differentiate situations when file doesnt exist and is forbidden
ByteArray *HttpServer::GetFileByPath(const char *path)
{
    char *file_path = AddPathToStaticPrefix(path);

    ByteArray *file = read_file(file_path);
    free(file_path);

    return file;
}


// GetFileStat returns -1 if the file has not been found and 0 in case of success
int HttpServer::GetFileStat(const char *path, struct stat *_stat) const
{
    char *full_path = AddPathToStaticPrefix(path);

    if (-1 == stat(full_path, _stat)) {
        free(full_path);

        if (errno == ENOENT) {
            return -1;
        }

        LOG_E("http: could not stat file %s: %s", path, strerror(errno));
        throw StatEx(strerror(errno));
    }

    free(full_path);

    return 0;
}


bool HttpServer::Init()
{
    if (!TcpServer::Init()) {
        return false;
    }

    return true;
}


HttpServer::HttpServer(const Config &cfg, sue_event_selector &selector, Photohosting *_photohosting)
    : TcpServer(cfg, selector),
    path_to_static_len(0),
    photohosting(_photohosting)
{
    path_to_static = strdup(cfg.path_to_static);
    assert(path_to_static);
    path_to_static_len = strlen(path_to_static);

    path_to_css = cfg.path_to_css;
    assert(path_to_css);
}


HttpServer::~HttpServer()
{
    if (path_to_static) free(path_to_static);
    if (path_to_css) free(path_to_css);
}

