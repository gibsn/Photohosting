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


TcpServer::TcpServer(const Config &cfg)
    : workers(NULL),
    n_sessions(0),
    sessions(NULL)
{
    sue_event_selector_init(&selector);
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

    return true;
}


bool TcpServer::Listen()
{
    if (listen(listen_fd_h.fd, 5)) {
        LOG_E("tcp: could not start listening: %s", strerror(errno));
        return false;
    }

    LOG_I("tcp: listening on %s:%d", addr, port);

    is_slave = false;
    int pid;
    for (int i = 0; i < n_workers; ++i) {
        pid = fork();

        srand(time(NULL) ^ (getpid() << 16));

        if (pid == -1) {
            LOG_E("tcp: could not fork a worker: %s", strerror(errno));
            return false;
        }

        if (pid != 0) {
            workers[i].pid = pid;
            LOG_I("tcp: forked a worker with PID %d", pid);
        } else {
            my_pid = getpid();
            is_slave = true;
            break;
        }
    }

    return true;
}

void TcpServer::FdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x)
{
    TcpSession *session = (TcpSession *)fd_h->userdata;
    TcpServer *server = session->GetTcpServer();
    server->FdHandler(session, r, w, x);
}

void TcpServer::FdHandler(TcpSession *session, int r, int w, int x)
{
    if (r) {
        ProcessRead(session);
    }

    if (w) {
        ProcessWrite(session);
    }

    // TODO wtf is x?
    if (x) {
    }

    if (session->GetWantToClose()) {
        CloseSession(session);
    }
}

void TcpServer::ToutHandlerCb(sue_timeout_handler *tout_h)
{
    TcpSession *session = (TcpSession *)tout_h->userdata;
    TcpServer *server = session->GetTcpServer();
    server->ToutHandler(session);
}

void TcpServer::ToutHandler(TcpSession *session)
{
    LOG_I("tcp: closing idle connection from %s [fd=%d]",
        session->GetSAddr(), session->GetFd());

    CloseSession(session);
}

void TcpServer::ProcessRead(TcpSession *session)
{
    session->ResetIdleTout(&selector);

    LOG_D("tcp: got data from %s [fd=%d]", session->GetSAddr(), session->GetFd());

    if (!session->ReadToBuf()) {
        LOG_I("tcp: client from %s has closed the connection [fd=%d]",
            session->GetSAddr(), session->GetFd());

        session->SetWantToClose(true);
        return;
    }

    if (!session->ProcessRequest()) {
        session->SetWantToClose(true);
    }
}


void TcpServer::ProcessWrite(TcpSession *session)
{
    LOG_D("tcp: sending data to %s [fd=%d]", session->GetSAddr(), session->GetFd());

    if (!session->Flush()) {
        session->SetWantToClose(true);
        return;
    }

    LOG_D("tcp: successfully sent data to %s [fd=%d]", session->GetSAddr(), session->GetFd());
}


void TcpServer::Serve()
{
    sue_event_selector_go(&selector);
}


// TODO do i really need sessions here? if i do change to slist then
void TcpServer::CloseSession(TcpSession *session)
{
    sue_event_selector_remove_fd_handler(&selector, session->GetFdHandler());
    sue_event_selector_remove_timeout_handler(&selector, session->GetToutHandler());

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

    server->CreateSession(tcp_session);
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

    TcpSession *new_session = new TcpSession(client_s_addr, this, fd, FdHandlerCb, ToutHandlerCb);
    new_session->InitFdHandler(&selector);
    new_session->InitIdleTout(&selector);

    sessions[n_sessions] = new_session;
    n_sessions++;

    return new_session;
}


void TcpServer::CreateSession(TcpSession *tcp_session)
{
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
}


bool TcpServer::ListenAndServe()
{
    if (!Listen()) return false;

    if (is_slave) {
        Serve();
    } else {
        close(listen_fd_h.fd);
        Wait();
    }

    return true;
}
