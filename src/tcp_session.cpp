#include "tcp_session.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"


TcpSession::TcpSession()
    : fd(0),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    s_addr(NULL),
    want_to_close(false)
{
}

TcpSession::TcpSession(int _fd, char *_s_addr)
    : fd(_fd),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    s_addr(NULL),
    want_to_close(false)
{
    s_addr = strdup(_s_addr);
}


TcpSession::~TcpSession()
{
    if (fd) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    if (write_buf) free(write_buf);
    if (s_addr) free(s_addr);
}


void TcpSession::TruncateReadBuf()
{
    read_buf_len = 0;
}


bool TcpSession::Read()
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


bool TcpSession::Send()
{
    int n = write(fd, write_buf, write_buf_len);
    free(write_buf);
    write_buf = NULL;
    write_buf_len = 0;

    if (n <= 0) {
        LOG_E("Some error occurred while writing to %s (%d)", s_addr, fd);
        return false;
    }

    return true;
}


char *TcpSession::GetReadBuf() const
{
    char *buf = (char *)malloc(read_buf_len + 1);
    memcpy(buf, read_buf, read_buf_len);
    buf[read_buf_len] = '\0';

    return buf;
}


void TcpSession::SetWriteBuf(char *buf)
{
    write_buf_len = strlen(buf);
    write_buf = (char *)malloc(write_buf_len);
    memcpy(write_buf, buf, write_buf_len);
}


