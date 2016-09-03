#include "http_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
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
    clients = new Client[MAX_CLIENTS];
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


bool HttpServer::ProcessRequest(Client &client)
{
    LOG_D("Got data from %s", client.s_addr);

    //to be deleted
    char buf[1024];
    int buf_len = read(client.fd, buf, sizeof(buf));
    if (buf_len == 0) {
        DeleteClient(client.fd);
        return false;
    }

    LOG_D("Processing request from %s", client.s_addr);

    //if ready to write
    FD_SET(client.fd, &so.writefds);

    return true;
}


bool HttpServer::SendResponse(Client &client)
{
    LOG_D("Sending response to %s", client.s_addr);
    write(client.fd, "Hello\n", 6);
    FD_CLR(client.fd, &so.writefds);
    return true;
}


void HttpServer::Serve()
{
    fd_set readfds, writefds;
    writefds = so.writefds;
    readfds = so.readfds;

    while(select(so.nfds, &readfds, &writefds, NULL, NULL) > 0) {
        if (FD_ISSET(listen_fd, &readfds)) AcceptNewConnection();

        //gotta process disconnected clients
        for (int i = 0; i < n_clients; ++i) {
            if (FD_ISSET(clients[i].fd, &readfds)) ProcessRequest(clients[i]);
            if (FD_ISSET(clients[i].fd, &writefds)) SendResponse(clients[i]);
        }

        writefds = so.writefds;
        readfds = so.readfds;
    }
    perror("select");
}


void HttpServer::AddNewClient(int fd, char *s_addr)
{
    clients[n_clients].fd = fd;

    //not sure yet how long buf should be
    clients[n_clients].read_buf = NULL;
    clients[n_clients].write_buf = NULL;

    clients[n_clients].s_addr = strdup(s_addr);

    n_clients++;

    FD_SET(fd, &so.readfds);
    if (fd + 1 > so.nfds) so.nfds = fd + 1;
}


void HttpServer::DeleteClient(int fd)
{
    int i;
    for (i = 0; i < n_clients; ++i) {
        if (clients[i].fd == fd) break;
    }

    LOG_I("Closing connection for %s", clients[i].s_addr);

    clients[i].~Client();
    memmove(clients + i, clients + i + 1, n_clients - i - 1);

    FD_CLR(fd, &so.readfds);
    FD_CLR(fd, &so.writefds);

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


Client::Client()
    : fd(0),
    read_buf(NULL),
    write_buf(NULL),
    s_addr(NULL)
{
}


Client::~Client()
{
    if (fd) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    if (read_buf) free(read_buf);
    if (write_buf) free(write_buf);

    if (s_addr) free(s_addr);
}




