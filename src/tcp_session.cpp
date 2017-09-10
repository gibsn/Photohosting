#include "tcp_session.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"


TcpSession::TcpSession()
    : active(false),
    fd(0),
    select_driver(NULL),
    session_driver(NULL),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    write_buf_offset(0),
    s_addr(NULL),
    want_to_close(false)
{
}

TcpSession::TcpSession(int _fd, char *_s_addr, SelectLoopDriver *_sd)
    : active(true),
    fd(_fd),
    select_driver(_sd),
    session_driver(NULL),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    write_buf_offset(0),
    s_addr(NULL),
    want_to_close(false)
{
    s_addr = strdup(_s_addr);
}


bool TcpSession::ProcessRequest()
{
    return session_driver->ProcessRequest();
}


void TcpSession::Close()
{
    LOG_I("tcp: closing session for %s (%d)", s_addr, fd);

    if (fd) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
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
    int n = read(fd, read_buf + read_buf_len, READ_BUF_SIZE - read_buf_len);

    if (n == 0) return false;

    if (n < 0) {
        if (errno != ECONNRESET) {
            LOG_E("tcp: some error occurred while reading from %s (%d): %s",
                    s_addr, fd, strerror(errno));
        }

        return false;
    }

    read_buf_len += n;

    return true;
}


bool TcpSession::Flush()
{
    int n = write(fd, write_buf + write_buf_offset, write_buf_len - write_buf_offset);

    if (n <= 0) {
        LOG_E("tcp: some error occurred while writing to %s (%d): %s",
                s_addr, fd, strerror(errno));

        return false;
    }

    write_buf_offset += n;

    if (write_buf_offset == write_buf_len) {
        TruncateWriteBuf();
    } else {
        select_driver->RequestWriteForFd(fd);
    }

    return true;
}


ByteArray *TcpSession::GetReadBuf() const
{
    return new ByteArray((char *)read_buf, read_buf_len);
}


void TcpSession::Send(ByteArray *buf)
{
    write_buf_len = buf->size;
    write_buf = (char *)malloc(buf->size);
    memcpy(write_buf, buf->data, buf->size);
    select_driver->RequestWriteForFd(fd);
}


