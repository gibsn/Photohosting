#include "http_server.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"


extern int my_pid;


HttpServer::HttpServer()
    : addr(NULL),
    workers(NULL),
    listen_fd(0),
    n_clients(0),
    clients(NULL)
{}


HttpServer::~HttpServer()
{
    if (addr) free(addr);
    if (workers) delete[] workers;
    if (clients) delete[] clients;
}


void HttpServer::SetArgs(const Config &cfg)
{
    addr = strdup(cfg.addr);
    port = cfg.port;
    n_workers = cfg.n_workers;
}


void HttpServer::Init()
{
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    if (listen_fd == -1) {
        LOG_E("Could not create a socket to listen on: %s",
            strerror(errno));
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sin_addr;
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sin_addr.sin_addr.s_addr);

    if (bind(listen_fd, (struct sockaddr *)&sin_addr, sizeof(sin_addr) )) {
        LOG_E("Could not bind to %s:%d: %s", addr, port, strerror(errno));
        exit(1);
    }

    workers = new Worker[n_workers];
    clients = new HttpClient[MAX_CLIENTS];
}


void HttpServer::Listen()
{
    if (listen(listen_fd, 5)) {
        LOG_E("Could not start listening: %s", strerror(errno));
        exit(1);
    }

    LOG_I("Started listening on %s:%d", addr, port);
    FD_SET(listen_fd, &so.readfds);
    so.nfds = listen_fd + 1;

    is_slave = false;
    int pid;
    for (int i = 0; i < n_workers; ++i) {
        pid = fork();

        if (pid == -1) {
            LOG_E("Could not fork a worker: %s", strerror(errno));
            exit(1);
        } else if (pid != 0) {
            workers[i].pid = pid;
            LOG_I("Forked a worker with PID %d", pid);
        } else {
            my_pid = getpid();
            is_slave = true;
            break;
        }
    }
}



void HttpServer::RespondOk(HttpClient &client)
{
    client.response.code = http_ok;
    client.response.minor_version = client.request.minor_version;

    client.response.AddDateHeader();
    client.response.AddHeader(
        "Connection", client.request.keep_alive? "keep-alive" : "close");

    client.response.body_len = 0;
    client.response.body = NULL;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", client.response.body_len);
    client.response.AddHeader("Content-Length", buf);

    SendResponse(client);
}


void HttpServer::ProcessRequest(HttpClient &client)
{
    // hexdump((uint8_t *)client.read_buf, client.read_buf_len);
    if (!ParseHttpRequest(client)) return;

    LOG_D("Processing request from %s (%d)", client.s_addr, client.fd);

    RespondOk(client);
}


bool HttpServer::ParseHttpRequest(HttpClient &client)
{
    int res = phr_parse_request(client.read_buf,
                                client.read_buf_len,
                                (const char**)&client.request.method,
                                &client.request.method_len,
                                (const char **)&client.request.path,
                                &client.request.path_len,
                                &client.request.minor_version,
                                client.request.headers,
                                &client.request.n_headers,
                                0);

    if (res == -1) {
        LOG_E("Failed to parse HTTP-Request from %s (%d)",
            client.s_addr, client.fd);
        client.TruncateReadBuf();

        return false;
    } else if (res == -2) {
        LOG_W("Got an incomplete HTTP-Request from %s (%d), "
            "waiting for the rest", client.s_addr, client.fd);

        return false;
    }

    client.request.body = client.read_buf + res;
    ProcessHeaders(client);

    return true;
}


void HttpServer::ProcessHeaders(HttpClient &client)
{
#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

    struct phr_header *headers = client.request.headers;
    for (int i = 0; i < client.request.n_headers; ++i) {

        if (CMP_HEADER("Connection")) {
            client.request.keep_alive =
                headers[i].value_len > 0 &&
                tolower(headers[i].value[0]) == 'k';
        } else if (CMP_HEADER("Content-Length")) {
            client.request.body_len = strtol(headers[i].value, NULL, 10);
        }
    }

#undef CMP_HEADER
}


void HttpServer::SendResponse(HttpClient &client)
{
    FD_SET(client.fd, &so.writefds);

    int len = 0;
    char *response = client.response.GenerateResponse(len);

    client.write_buf = new char[len];
    memcpy(client.write_buf, response, len);
    client.write_buf_len += len;

    free(response);
}


void HttpServer::Serve()
{
    fd_set readfds, writefds;
    writefds = so.writefds;
    readfds = so.readfds;

    while(select(so.nfds, &readfds, &writefds, NULL, NULL) > 0) {
        if (FD_ISSET(listen_fd, &readfds)) AcceptNewConnection();

        for (int i = 0; i < n_clients; ++i) {
            if (FD_ISSET(clients[i].fd, &readfds)) {
                LOG_D("Got data from %s (%d)",
                    clients[i].s_addr, clients[i].fd);

                if (clients[i].Read()) {
                    ProcessRequest(clients[i]);
                } else {
                    DeleteClient(clients[i].fd);
                }
            }

            if (FD_ISSET(clients[i].fd, &writefds)) {
                LOG_D("Sending data to %s (%d)",
                    clients[i].s_addr, clients[i].fd);

                if (clients[i].Send()) {
                    LOG_D("Successfully sent data to %s (%d)",
                        clients[i].s_addr, clients[i].fd);

                    FD_CLR(clients[i].fd, &so.writefds);
                    DeleteClient(clients[i].fd);
                    // if (!clients[i].keep_alive) DeleteClient(clients[i].fd);
                } else {
                    DeleteClient(clients[i].fd);
                }
            }
        }

        writefds = so.writefds;
        readfds = so.readfds;
    }
}


void HttpServer::AddNewClient(int fd, char *s_addr)
{
    clients[n_clients].fd = fd;

    clients[n_clients].read_buf_len = 0;

    clients[n_clients].write_buf = NULL;
    clients[n_clients].write_buf_len = 0;

    clients[n_clients].s_addr = strdup(s_addr);

    n_clients++;

    FD_SET(fd, &so.readfds);
    if (fd + 1 > so.nfds) so.nfds = fd + 1;
}


void HttpServer::DeleteClient(int fd)
{
    int i;
    //use binary search here
    for (i = 0; i < n_clients; ++i) {
        if (clients[i].fd == fd) break;
    }

    LOG_I("Closing connection for %s (%d)", clients[i].s_addr, clients[i].fd);


    clients[i].~HttpClient();
    clients[i] = HttpClient();

    memmove(clients + i, clients + i + 1, sizeof(HttpClient) * (n_clients - i - 1));

    if (FD_ISSET(fd, &so.readfds)) FD_CLR(fd, &so.readfds);
    if (FD_ISSET(fd, &so.writefds)) FD_CLR(fd, &so.writefds);

    n_clients--;
}


void HttpServer::AcceptNewConnection()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

    if (fd == -1) {
        LOG_E("Could not accept a new connection: %s", strerror(errno));
        return;
    }

    char *client_s_addr = inet_ntoa(client_addr.sin_addr);
    LOG_I("Accepted a new connection from %s", client_s_addr);

    if (n_clients < MAX_CLIENTS) {
        AddNewClient(fd, client_s_addr);
    } else {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        LOG_W("Declined connection from %s due to reaching MAX_CLIENTS",
            client_s_addr);
    }
}


void HttpServer::Wait()
{
    pid_t pid;
    for (int i = 0; i < n_workers; ++i) {
        pid = wait(NULL);
        LOG_I("Worker %d has exited", pid);
    }
}


void HttpServer::ListenAndServe()
{
    Listen();

    if (is_slave) {
        Serve();
    } else {
        close(listen_fd);
        Wait();
    }
}


SelectOpts::SelectOpts()
    : nfds(0)
{
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
}


