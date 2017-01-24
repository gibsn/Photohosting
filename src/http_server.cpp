#include "http_server.h"

#include <assert.h>
#include <ctype.h>
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
        int file_path_len = strlen(path) - sizeof "/static";
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


HttpServer::HttpServer()
    : path_to_static(0),
    path_to_static_len(0)
{
}


HttpServer::HttpServer(const Config &cfg)
    : TcpServer(cfg),
    path_to_static_len(0)
{
    path_to_static = strdup(cfg.path_to_static);
    assert(path_to_static);
    path_to_static_len = strlen(path_to_static);
}


HttpServer::~HttpServer()
{
    if (path_to_static) free(path_to_static);
}

