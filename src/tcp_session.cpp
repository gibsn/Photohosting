#include "tcp_session.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"


// TcpSession::TcpSession()
//     : active(false),
//     select_driver(NULL),
//     session_driver(NULL),
//     read_buf_len(0),
//     write_buf(NULL),
//     write_buf_len(0),
//     write_buf_offset(0),
//     s_addr(NULL),
//     want_to_close(false)
// {
// }

TcpSession::TcpSession(int _fd, char *_s_addr, TcpServer *_tcp_server)
    : active(true),
    s_addr(NULL),
    session_driver(NULL),
    tcp_server(_tcp_server),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    write_buf_offset(0),
    want_to_close(false)
{
    memset(&fd_h, 0, sizeof(fd_h));
    fd_h.fd = _fd;
    fd_h.want_read = 1;

    s_addr = strdup(_s_addr);
}


bool TcpSession::ProcessRequest()
{
    return session_driver->ProcessRequest();
}


void TcpSession::Close()
{
    LOG_I("tcp: closing session for %s [fd=%d]", s_addr, fd_h.fd);

    if (fd_h.fd) {
        shutdown(fd_h.fd, SHUT_RDWR);
        close(fd_h.fd);
    }

    active = false;
}


TcpSession::~TcpSession()
{
    if (active) this->Close();
    session_driver->Close();

    delete session_driver;

    free(write_buf);
    free(s_addr);
}


void TcpSession::TruncateReadBuf()
{
    read_buf_len = 0;
}


void TcpSession::TruncateWriteBuf()
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
        TruncateWriteBuf();
        fd_h.want_write = 0;
    }

    return true;
}


ByteArray *TcpSession::GetReadBuf()
{
    ByteArray *buf_copy = new ByteArray((char *)read_buf, read_buf_len);
    TruncateReadBuf();
    return buf_copy;
}


// TODO why not by reference and copy?
void TcpSession::Send(ByteArray *buf)
{
    write_buf_len = buf->size;
    write_buf = (char *)malloc(buf->size);
    memcpy(write_buf, buf->data, buf->size);

    fd_h.want_write = 1;
}


