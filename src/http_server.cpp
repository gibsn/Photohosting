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
#include <unistd.h>

#include "common.h"
#include "http_request.h"
#include "http_response.h"
#include "http_session.h"
#include "log.h"


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

    int n;
    int fd = open(full_path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        LOG_E("%s", strerror(errno));
        goto err;
    }

    n = write(fd, file->data, file->size);

    if (file->size != n) {
        LOG_E("Could not write file %s", full_path);
        goto err;
    }

    return full_path;

err:
    if (full_path) free(full_path);

    return NULL;
}


HttpServer::HttpServer()
    : path_to_static(NULL),
    path_to_static_len(0),
    path_to_tmp_files(NULL)
{
}


HttpServer::HttpServer(const Config &cfg)
    : TcpServer(cfg),
    path_to_static_len(0)
{
    path_to_static = strdup(cfg.path_to_static);
    assert(path_to_static);
    path_to_static_len = strlen(path_to_static);

    path_to_tmp_files = strdup(cfg.path_to_tmp_files);
    assert(path_to_tmp_files);
}


HttpServer::~HttpServer()
{
    if (path_to_static) free(path_to_static);
}

