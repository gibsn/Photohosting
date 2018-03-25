#include "tcp_session.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"


TcpSession::TcpSession(
        char *_s_addr,
        TcpServer *_tcp_server,
        int _fd,
        sue_fd_handler_cb_t fd_cb,
        sue_tout_handler_cb_t tout_cb)
  : s_addr(NULL),
    app_layer_session(NULL),
    tcp_server(_tcp_server),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    write_buf_offset(0),
    want_to_close(false)
{
    s_addr = strdup(_s_addr);

    memset(&fd_h, 0, sizeof(fd_h));
    fd_h.fd = _fd;
    fd_h.userdata = this;
    fd_h.handle_fd_event = fd_cb;

    memset(&tout_h, 0, sizeof(tout_h));
    tout_h.userdata = this;
    tout_h.handle_timeout = tout_cb;
}


void TcpSession::ProcessRequest()
{
    app_layer_session->ProcessRequest();
}


void TcpSession::Close()
{
    LOG_I("tcp: closing session for %s [fd=%d]", s_addr, fd_h.fd);

    if (fd_h.fd) {
        shutdown(fd_h.fd, SHUT_RDWR);
        close(fd_h.fd);
    }
}


TcpSession::~TcpSession()
{
    Close();

    free(write_buf);
    free(s_addr);
}


void TcpSession::ResetReadBuf()
{
    read_buf_len = 0;
}

void TcpSession::InitFdHandler(sue_event_selector *selector)
{
    fd_h.want_read = 1;
    sue_event_selector_register_fd_handler(selector, &fd_h);
}

void TcpSession::InitIdleTout(sue_event_selector *selector)
{
    sue_timeout_handler_set_from_now(&tout_h, IDLE_TIMEOUT, 0);
    sue_event_selector_register_timeout_handler(selector, &tout_h);
}

void TcpSession::ResetIdleTout(sue_event_selector *selector)
{
    sue_event_selector_remove_timeout_handler(selector, &tout_h);
    InitIdleTout(selector);
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
            LOG_E("tcp: some error occurred while reading from %s [fd=%d]: %s",
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
        LOG_E("tcp: some error occurred while writing to %s [fd=%d]: %s",
                s_addr, fd_h.fd, strerror(errno));

        return false;
    }

    write_buf_offset += n;

    if (write_buf_offset == write_buf_len) {
        ResetWriteBuf();
        fd_h.want_write = 0;
    }

    return true;
}


void TcpSession::Shutdown()
{
    shutdown(fd_h.fd, SHUT_RD);
    want_to_close = true;
}


ByteArray *TcpSession::GetReadBuf()
{
    ByteArray *buf_copy = new ByteArray((char *)read_buf, read_buf_len);
    ResetReadBuf();
    return buf_copy;
}


void TcpSession::Send(ByteArray *buf)
{
    write_buf_len = buf->size;
    write_buf = (char *)malloc(buf->size);
    memcpy(write_buf, buf->data, buf->size);

    fd_h.want_write = 1;
}


