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
    LOG_I("Closing TCP-session for %s (%d)", s_addr, fd);

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

    if (session_driver) delete session_driver;

    if (write_buf) free(write_buf);
    if (s_addr) free(s_addr);
}


void TcpSession::TruncateReadBuf()
{
    read_buf_len = 0;
}


bool TcpSession::ReadToBuf()
{
    int n = read(fd, read_buf + read_buf_len, READ_BUF_SIZE - read_buf_len);

    if (n == 0) return false;

    if (n < 0) {
        if (errno != ECONNRESET) {
            LOG_E("Some error occurred while reading from %s (%d)", s_addr, fd);
        }

        return false;
    }

    read_buf_len += n;

    return true;
}


bool TcpSession::Flush()
{
    int ret = true;
    int offset = 0;
    int n;
    while (write_buf_len != 0) {
        n = write(fd, write_buf + offset, write_buf_len);

        if (n <= 0) {
            perror("write");
            LOG_E("Some error occurred while writing to %s (%d)", s_addr, fd);
            ret = false;
            break;
        }

        offset += n;
        write_buf_len -= n;
    }

    free(write_buf);

    write_buf = NULL;
    write_buf_len = 0;

    return ret;
}


// TODO: use bytearray here
ByteArray *TcpSession::GetReadBuf() const
{
    return new ByteArray((char *)read_buf, read_buf_len);
}


void TcpSession::Send(ByteArray *buf)
{
    write_buf_len = buf->size;
    write_buf = (char *)malloc(buf->size);
    memcpy(write_buf, buf->data, buf->size);
    select_driver->SetWantToWrite(fd);
}


