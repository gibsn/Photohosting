#include "http_server.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "log.h"


HttpServer::HttpServer()
    : addr(0),
    listen_fd(0)
{}


HttpServer::~HttpServer()
{
    if (addr) free(addr);
}


void HttpServer::SetArgs(const Config &cfg)
{
    addr = strdup(cfg.addr);
    port = cfg.port;
    n_workers = cfg.n_workers;
}


void HttpServer::Listen()
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
    //fix this
    sin_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&sin_addr, sizeof(sin_addr) )) {
        LOG_E("Could not bind to %s:%d: %s", addr, port, strerror(errno));
        exit(1);
    }

    if (listen(listen_fd, 5)) {
        LOG_E("Could not start listening: %s", strerror(errno));
        exit(1);
    }
    LOG_I("Started listening on %s:%d", addr, port);

    slave = false;
    int pid;
    for (int i = 0; i < n_workers; ++i) {
        pid = fork();

        if (pid == -1) {
            LOG_E("Could not fork a worker: %s", strerror(errno));
            exit(1);
        } else if (pid != 0) {
            LOG_I("Forked a worker with PID %d", pid);
        } else {
            slave = true;
            break;
        }
    }
}


void HttpServer::Serve()
{
    int fd;
    if ((fd = accept(listen_fd, NULL, NULL)) != -1) {
        LOG_E("Could not accept a new connection: %s", strerror(errno));
    }

    //remember the ip
    LOG_I("Got a new connection from");
}


void HttpServer::Wait()
{
    //wait and kill zombies
}


void HttpServer::ListenAndServe()
{
    Listen();

    if (slave) {
        Serve();
    } else {
        Wait();
    }
}







