#include "tcp_session.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"


TcpSession::TcpSession(
    char *_s_addr,
    int _fd,
    TcpSessionManagerBridge *_session_manager,
    sue_event_selector &_selector
):
    selector(_selector),
    application_level_handler(NULL),
    s_addr(NULL),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    write_buf_offset(0),
    want_to_close(false)
{
    s_addr = strdup(_s_addr);

    userdata.session = this;
    userdata.session_manager = _session_manager;

    memset(&fd_h, 0, sizeof(fd_h));
    fd_h.fd = _fd;
    fd_h.userdata = &userdata;
    fd_h.handle_fd_event = FdHandlerCb;
    fd_h.want_read = 1;
    sue_event_selector_register_fd_handler(&selector, &fd_h);

    memset(&tout_h, 0, sizeof(tout_h));
    tout_h.userdata = &userdata;
    tout_h.handle_timeout = TimeoutHandlerCb;
    sue_timeout_handler_set_from_now(&tout_h, IDLE_TIMEOUT, 0);
    sue_event_selector_register_timeout_handler(&selector, &tout_h);
}


void TcpSession::OnRead()
{
    ResetIdleTimeout();

    LOG_D("tcp: got data from %s [fd=%d]", s_addr, fd_h.fd);

    if (!ReadToBuf()) {
        LOG_I("tcp: client from %s has closed the connection [fd=%d]", s_addr, fd_h.fd);
        return WantToClose();
    }

    if (application_level_handler) {
        application_level_handler->OnRead();
    }
}


void TcpSession::OnWrite()
{
    if (!Flush()) {
        return WantToClose();
    }

    LOG_D("tcp: successfully sent data to %s [fd=%d]", s_addr, fd_h.fd);

    if (application_level_handler) {
        application_level_handler->OnWrite();
    }
}


void TcpSession::Close()
{
    LOG_I("tcp: closing session [addr=%s; fd=%d]", s_addr, fd_h.fd);

    if (fd_h.fd) {
        shutdown(fd_h.fd, SHUT_RDWR);
        close(fd_h.fd);
    }
}


TcpSession::~TcpSession()
{
    Close();

    sue_event_selector_remove_fd_handler(&selector, &fd_h);
    sue_event_selector_remove_timeout_handler(&selector, &tout_h);

    free(write_buf);
    free(s_addr);
}


void TcpSession::ResetReadBuf()
{
    read_buf_len = 0;
}

void TcpSession::FdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x)
{
    SueTcpSessionHandlerUserData *userdata = (SueTcpSessionHandlerUserData *)fd_h->userdata;
    TcpSession *session = userdata->session;
    TcpSessionManagerBridge *session_manager = userdata->session_manager;

    if (r) {
        session_manager->OnRead(session);
    }
    if (w) {
        session_manager->OnWrite(session);
    }
    if (x) {
        session_manager->OnX(session);
    }
}

void TcpSession::TimeoutHandlerCb(sue_timeout_handler *tout_h)
{
    SueTcpSessionHandlerUserData *userdata = (SueTcpSessionHandlerUserData *)tout_h->userdata;
    TcpSession *session = userdata->session;
    TcpSessionManagerBridge *session_manager = userdata->session_manager;

    session_manager->OnTimeout(session);
}

void TcpSession::ResetIdleTimeout()
{
    sue_event_selector_remove_timeout_handler(&selector, &tout_h);
    sue_timeout_handler_set_from_now(&tout_h, IDLE_TIMEOUT, 0);
    sue_event_selector_register_timeout_handler(&selector, &tout_h);
}

void TcpSession::ResetWriteBuf()
{
    free(write_buf);
    write_buf = NULL;
    write_buf_len = 0;
    write_buf_offset = 0;
}


bool TcpSession::ReadToBuf()
{
    int n = read(fd_h.fd, read_buf + read_buf_len, READ_BUF_SIZE - read_buf_len);

    if (n == 0) return false;

    if (n < 0) {
        if (errno != ECONNRESET) {
            LOG_E("tcp: read error [addr=%s, fd=%d]: %s",
                    s_addr, fd_h.fd, strerror(errno));
        }

        return false;
    }

    read_buf_len += n;

    return true;
}


bool TcpSession::Flush()
{
    int n = write(fd_h.fd, write_buf + write_buf_offset, write_buf_len - write_buf_offset);

    if (n <= 0) {
        LOG_E("tcp: write error [addr=%s; fd=%d]: %s", s_addr, fd_h.fd, strerror(errno));
        return false;
    }

    write_buf_offset += n;

    if (write_buf_offset == write_buf_len) {
        ResetWriteBuf();
        fd_h.want_write = 0;
    }

    return true;
}

ByteArray *TcpSession::Read()
{
    ByteArray *buf_copy = new ByteArray((char *)read_buf, read_buf_len);
    ResetReadBuf();
    return buf_copy;
}


void TcpSession::Write(ByteArray *buf)
{
    write_buf_len = buf->size;
    write_buf = (char *)malloc(buf->size);
    memcpy(write_buf, buf->data, buf->size);

    fd_h.want_write = 1;
}
