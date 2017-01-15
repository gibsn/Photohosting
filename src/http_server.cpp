#include "http_server.h"

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
#include "log.h"


HttpSession *HttpServer::GetSessionByFd(int fd)
{
    for (int i = 0; i < n_sessions; ++i) {
        if (sessions[i]->GetFd() == fd) return sessions[i];
    }

    return sessions[0];
}


void HttpServer::Respond(int fd, HttpResponse *response)
{
    TcpServer::WriteTo(fd, response->response, response->response_len);
}


bool HttpServer::ProcessRequest(int fd)
{
    bool ret = true;

    HttpSession *session = GetSessionByFd(fd);

    char *read_buf = TcpServer::ReadFrom(fd);
    // hexdump((uint8_t *)read_buf, strlen(read_buf));
    if (!session->ParseHttpRequest(read_buf)) {
        ret = false;
        goto fin;
    }

    LOG_D("Processing HTTP-request");

#define METHOD_IS(str)\
    session->GetRequest()->method_len == strlen(str) && \
    !strncmp(session->GetRequest()->method, str, session->GetRequest()->method_len)

    if (METHOD_IS("GET")) {
        ProcessGetRequest(session);
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest(session);
    } else {
        Respond(fd, session->RespondNotFound());
    }

    if (!session->GetKeepAlive()) WantToCloseSession(fd);

fin:
    session->PrepareForNextRequest();
    free(read_buf);
    return ret;
#undef METHOD_IS
}


#define LOCATION_IS(str)                              \
    session->GetRequest()->path_len == strlen(str) && \
    !strncmp(session->GetRequest()->path, str, session->GetRequest()->path_len)

#define LOCATION_CONTAINS(str)  \
    strnstr(session->GetRequest()->path, str, session->GetRequest()->path_len)

#define LOCATION_STARTS_WITH(str)  \
    session->GetRequest()->path == \
    strnstr(session->GetRequest()->path, str, session->GetRequest()->path_len)

void HttpServer::ProcessGetRequest(HttpSession *session)
{
    const HttpRequest *req = session->GetRequest();
    char *path = strndup(req->path, req->path_len);

    if (LOCATION_IS("/")) {
        Respond(session->GetFd(), session->RespondOk());
    } else if (LOCATION_CONTAINS("/../")) {
        Respond(session->GetFd(), session->RespondBadRequest());
    } else if (LOCATION_STARTS_WITH("/static/")) {
        char *file_path = path + sizeof "/static/" - 1;

        if (file_exists(file_path)) {
            Respond(session->GetFd(), session->RespondFile(file_path));
        } else {
            Respond(session->GetFd(), session->RespondNotFound());
        }
    } else {
        Respond(session->GetFd(), session->RespondNotFound());
    }

    free(path);
}
#undef LOCATION_IS
#undef LOCATION_CONTAINS
#undef LOCATION_STARTS_WITH


void HttpServer::ProcessPostRequest(HttpSession *session)
{
    Respond(session->GetFd(), session->RespondNotImplemented());
}


void HttpServer::CloseSession(int fd)
{
    TcpServer::CloseSession(fd);

    int i, curr_fd;
    for (i = 0; i < n_sessions; ++i) {
        curr_fd = sessions[i]->GetFd();
        if (curr_fd == fd) break;
    }

    LOG_I("Closing Http-session");

    delete sessions[i];

    for (int j = i + 1; j < n_sessions; ++j) {
        sessions[j - 1] = sessions[j];
    }
    sessions[n_sessions - 1] = NULL;
    n_sessions--;
}


int HttpServer::CreateNewSession()
{
    int fd = TcpServer::CreateNewSession();

    if (fd != -1) {
        sessions[n_sessions] = new HttpSession(fd);
        n_sessions++;
    } else {
        LOG_D("Could not open a new HTTP-session");
    }

    return fd;
}


void HttpServer::ListenAndServe()
{
    TcpServer::ListenAndServe();
}


void HttpServer::Init()
{
    TcpServer::Init();

    sessions = new HttpSession *[MAX_SESSIONS];
}


HttpServer::HttpServer()
    : n_sessions(0),
    sessions(0)
{
}


HttpServer::HttpServer(const Config &cfg)
    : TcpServer(cfg),
    n_sessions(0),
    sessions(0)
{
}


HttpServer::~HttpServer()
{
    if (sessions) {
        for (int i = 0; i < n_sessions; ++i) {
            delete sessions[i];
        }

        delete[] sessions;
    }
}

