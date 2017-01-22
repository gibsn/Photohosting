#include "tcp_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "log.h"


TcpServer::TcpServer()
    : addr(NULL),
    workers(NULL),
    n_sessions(0)
{
}


TcpServer::TcpServer(const Config &cfg)
    : workers(NULL),
    n_sessions(0)
{
    addr = strdup(cfg.addr);
    n_workers = cfg.n_workers;
    port = cfg.port;
}


TcpServer::~TcpServer()
{
    if (addr) free(addr);
    if (workers) delete[] workers;
    if (sessions) {
        for (int i = 0; i < MAX_SESSIONS; ++i) {
            if (sessions[i]) {
                delete sessions[i];
            }
        }

        delete[] sessions;
    }
}


void TcpServer::Init()
{
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    if (listen_fd == -1) {
        LOG_E("Could not create a socket to listen on: %s",
            strerror(errno));
        exit(1);
    }

    int args = fcntl(listen_fd, F_GETFD, 0);
    assert(args != -1);
    assert(fcntl(listen_fd, F_SETFL, args | O_NONBLOCK) != -1);

    int opt = 1;
    int r = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    assert(r != -1);
    r = setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    assert(r != -1);

    struct sockaddr_in sin_addr;
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sin_addr.sin_addr.s_addr);

    if (bind(listen_fd, (struct sockaddr *)&sin_addr, sizeof(sin_addr) )) {
        LOG_E("Could not bind to %s:%d: %s", addr, port, strerror(errno));
        exit(1);
    }

    workers = new Worker[n_workers];
    sessions = new TcpSession *[MAX_SESSIONS];
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        sessions[i] = NULL;
    }
}


void TcpServer::Listen()
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


bool TcpServer::ProcessRequest(int fd)
{
    return true;
}


void TcpServer::ProcessRead(const fd_set &readfds, TcpSession *session)
{
    if (FD_ISSET(session->GetFd(), &readfds)) {
        LOG_D("Got data from %s (%d)",
            session->GetSAddr(), session->GetFd());

        if (session->Read()) {
            if (!ProcessRequest(session->GetFd())) {
                CloseSession(session->GetFd());
            }
        } else {
            LOG_D("The client from %s has closed the connection (%d)",
                session->GetSAddr(), session->GetFd());
            CloseSession(session->GetFd());
        }
    }
}


void TcpServer::ProcessWrite(const fd_set &writefds, TcpSession *session)
{
    if (FD_ISSET(session->GetFd(), &writefds)) {
        LOG_D("Sending data to %s (%d)",
            session->GetSAddr(), session->GetFd());

        if (session->Send()) {
            LOG_D("Successfully sent data to %s (%d)",
                session->GetSAddr(), session->GetFd());

            FD_CLR(session->GetFd(), &so.writefds);
            if (session->GetWantToClose()) {
                CloseSession(session->GetFd());
            }
        } else {
            CloseSession(session->GetFd());
        }
    }
}


void TcpServer::Serve()
{
    fd_set readfds, writefds;
    writefds = so.writefds;
    readfds = so.readfds;

    while(select(so.nfds, &readfds, &writefds, NULL, NULL) > 0) {
        if (FD_ISSET(listen_fd, &readfds)) CreateNewSession();

        for (int i = 0; i < n_sessions; ++i) {
            if (sessions[i]) ProcessRead(readfds, sessions[i]);
            if (sessions[i]) ProcessWrite(writefds, sessions[i]);
        }

        writefds = so.writefds;
        readfds = so.readfds;
    }
}


void TcpServer::AddNewClient(int fd, char *s_addr)
{
    sessions[n_sessions] = new TcpSession(fd, s_addr);
    n_sessions++;

    FD_SET(fd, &so.readfds);
    if (fd + 1 > so.nfds) so.nfds = fd + 1;
}


void TcpServer::WantToCloseSession(int fd)
{
    TcpSession *session = GetSessionByFd(fd);
    session->SetWantToClose(true);
}


void TcpServer::CloseSession(int fd)
{
    int i;
    int max_fd = listen_fd;
    //TODO: use binary search here
    int curr_fd;
    for (i = 0; i < n_sessions; ++i) {
        curr_fd = sessions[i]->GetFd();
        if (curr_fd == fd) break;
        if (curr_fd > max_fd) max_fd = curr_fd;
    }

    LOG_I("Closing TCP-session for %s (%d)", sessions[i]->GetSAddr(), sessions[i]->GetFd());
    delete sessions[i];

    for (int j = i + 1; j < n_sessions; ++j) {
        if (sessions[j]->GetFd() > max_fd) max_fd = sessions[j]->GetFd();
        sessions[j - 1] = sessions[j];
    }
    sessions[n_sessions - 1] = NULL;
    so.nfds = max_fd + 1;

    if (FD_ISSET(fd, &so.readfds)) FD_CLR(fd, &so.readfds);
    if (FD_ISSET(fd, &so.writefds)) FD_CLR(fd, &so.writefds);

    n_sessions--;
}


static bool set_socket_blocking(int sock)
{
    int opts;

    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        LOG_E("Could not make socket blocking: %s", strerror(errno));
        return false;
    }
    opts = (opts & (~O_NONBLOCK));
    if (fcntl(sock, F_SETFL, opts) < 0) {
        LOG_E("Could not make socket blocking: %s", strerror(errno));
        return false;
    }

    return true;
}


int TcpServer::CreateNewSession()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

    if (fd == -1) {
        if (errno != EAGAIN)
            LOG_E("Could not accept a new connection: %s", strerror(errno));

        return -1;
    }

    //BSD's accept creates new socket with the same properties as the listening
    if (!set_socket_blocking(fd)) return -1;

    char *client_s_addr = inet_ntoa(client_addr.sin_addr);
    LOG_I("Accepted a new connection from %s", client_s_addr);

    if (n_sessions < MAX_SESSIONS) {
        AddNewClient(fd, client_s_addr);
    } else {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        LOG_W("Declined connection from %s due to reaching MAX_SESSIONS",
            client_s_addr);

        return -1;
    }

    return fd;
}


TcpSession *TcpServer::GetSessionByFd(int fd)
{
    for (int i = 0; i < n_sessions; ++i) {
        if (sessions[i]->GetFd() == fd) return sessions[i];
    }

    return sessions[0];
}


char *TcpServer::ReadFrom(int fd)
{
    TcpSession *session = GetSessionByFd(fd);
    char *buf = session->GetReadBuf();
    session->TruncateReadBuf();

    return buf;
}


void TcpServer::WriteTo(int fd, char *buf, int len)
{
    TcpSession *session = GetSessionByFd(fd);
    session->SetWriteBuf(buf, len);
    FD_SET(fd, &so.writefds);
}


void TcpServer::Wait()
{
    pid_t pid;
    int stat_loc;
    for (int i = 0; i < n_workers; ++i) {
        pid = wait(&stat_loc);
        if (WEXITSTATUS(stat_loc)) {
            LOG_E("Worker %d has exited with error", pid);
        } else {
            LOG_I("Worker %d has exited normally", pid);
        }
    }
}


void TcpServer::ListenAndServe()
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


