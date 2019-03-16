#include "tcp_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cfg.h"
#include "common.h"
#include "log.h"


TcpServer::TcpServer(const Config &cfg, sue_event_selector &s)
    : workers(NULL),
    selector(s),
    n_sessions(0),
    sessions(NULL)
{
    memset(&listen_fd_h, 0, sizeof(listen_fd_h));

    addr = strdup(cfg.addr);
    n_workers = cfg.n_workers;
    port = cfg.port;
}


TcpServer::~TcpServer()
{
    free(addr);
    delete[] workers;
    if (sessions) {
        for (int i = 0; i < n_sessions; ++i) {
            delete sessions[i];
        }

        delete[] sessions;
    }
}


bool TcpServer::Init()
{
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    if (listen_fd == -1) {
        LOG_E("tcp: could not create a listening socket: %s", strerror(errno));
        return false;
    }

    int args = fcntl(listen_fd, F_GETFD, 0);
    assert(args != -1);
    assert(fcntl(listen_fd, F_SETFL, args | O_NONBLOCK) != -1);

    int opt = 1;
    int r = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    assert(r != -1);

    struct sockaddr_in sin_addr;
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sin_addr.sin_addr.s_addr);

    if (bind(listen_fd, (struct sockaddr *)&sin_addr, sizeof(sin_addr) )) {
        LOG_E("tcp: could not bind to %s:%d: %s", addr, port, strerror(errno));
        return false;
    }

    workers = new Worker[n_workers];
    sessions = new TcpSession *[MAX_FD];

    listen_fd_h.fd = listen_fd;
    listen_fd_h.handle_fd_event = ListenFdHandlerCb;
    listen_fd_h.userdata = this;
    listen_fd_h.want_read = 1;

    sue_event_selector_register_fd_handler(&selector, &listen_fd_h);

    if (listen(listen_fd_h.fd, 5)) {
        LOG_E("tcp: could not start listening: %s", strerror(errno));
        return false;
    }

    if (n_workers) {
        if (!SpawnWorkers()) {
            return false;
        }

        if (is_slave) {
            LOG_I("tcp: listening on %s:%d", addr, port);
            return true;
        }

        close(listen_fd_h.fd);
        Wait();
    }

    return true;
}


bool TcpServer::SpawnWorkers()
{
    is_slave = false;

    for (int i = 0; i < n_workers; ++i) {
        int pid = fork();

        if (pid == -1) {
            LOG_E("tcp: could not fork a worker: %s", strerror(errno));
            return false;
        }

        if (!pid) {
            is_slave = true;
            my_pid = getpid();
            srand(time(NULL) ^ (my_pid << 16));

            return true;
        }

        workers[i].pid = pid;
        LOG_I("tcp: forked a worker with PID %d", pid);
    }

    return true;
}

void TcpServer::OnRead(TcpSession *session)
{
    session->OnRead();

    if (session->ShouldClose()) {
        application_level_session_manager->Close(session->GetApplicationLevelHandler());
        CloseTcpSession(session);
    }
}

void TcpServer::OnWrite(TcpSession *session)
{
    session->OnWrite();

    if (session->ShouldClose()) {
        application_level_session_manager->Close(session->GetApplicationLevelHandler());
        CloseTcpSession(session);
    }
}

void TcpServer::OnX(TcpSession *session)
{
}

void TcpServer::OnTimeout(TcpSession *session)
{
    LOG_I("tcp: closing idle connection [addr=%s; fd=%d]", session->GetAddr(), session->GetFd());

    application_level_session_manager->Close(session->GetApplicationLevelHandler());
    CloseTcpSession(session);
}

// TODO do i really need sessions here? if i do change to slist then
void TcpServer::CloseTcpSession(TcpSession *session)
{
    int i = 0;
    while (sessions[i++]->GetFd() != session->GetFd());

    delete sessions[i-1];

    for (; i < n_sessions; ++i) {
        sessions[i - 1] = sessions[i];
    }

    n_sessions--;
}


static bool set_socket_blocking(int sock)
{
    int opts;

    opts = fcntl(sock, F_GETFL);
    assert(opts > 0);

    opts = (opts & (~O_NONBLOCK));
    if (fcntl(sock, F_SETFL, opts) < 0) {
        LOG_E("tcp: could not enable blocking mode for socket: %s", strerror(errno));
        return false;
    }

    return true;
}

void TcpServer::ListenFdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x)
{
    TcpServer *server = (TcpServer *)fd_h->userdata;

    TcpSession *tcp_session = server->CreateTcpSession();
    if (!tcp_session) {
        return;
    }

    ApplicationLevelBridge *h = server->GetApplicationLevelSessionManager()->Create(tcp_session);
    tcp_session->SetApplicationLevelHandler(h);
}


TcpSession *TcpServer::CreateTcpSession()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd = accept(listen_fd_h.fd, (struct sockaddr *)&client_addr, &client_len);

    if (fd == -1) {
        if (errno != EAGAIN) {
            LOG_E("tcp: could not accept a new connection: %s", strerror(errno));
        }

        return NULL;
    }

    //BSD's accept creates new socket with the same properties as the listening
    if (!set_socket_blocking(fd)) return NULL;

    char *client_s_addr = inet_ntoa(client_addr.sin_addr);
    LOG_I("tcp: accepted a new connection from %s", client_s_addr);

    if (fd > MAX_FD) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        LOG_W("tcp: declined connection from %s due to reaching MAX_FD", client_s_addr);

        return NULL;
    }

    TcpSession *new_session = new TcpSession(client_s_addr, fd, this, selector);
    sessions[n_sessions++] = new_session;

    return new_session;
}


void TcpServer::Wait()
{
    pid_t pid;
    int stat_loc;
    for (int i = 0; i < n_workers; ++i) {
        pid = wait(&stat_loc);
        // TODO wrong condition
        if (WIFEXITED(stat_loc)) {
            LOG_I("tcp: worker %d has exited normally", pid);
        } else {
            LOG_E("tcp: worker %d has exited with error", pid);
        }
    }

    //TODO move forking to main
    LOG_I("tcp: all workers have exited");
    exit(EXIT_SUCCESS);
}
