#include "http_client.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "log.h"


HttpClient::HttpClient()
    : fd(0),
    read_buf_len(0),
    write_buf(NULL),
    write_buf_len(0),
    s_addr(NULL)
{
}


HttpClient::~HttpClient()
{
    if (fd) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    if (write_buf) delete[] write_buf;
    if (s_addr) free(s_addr);
}


bool HttpClient::Read()
{
    int n = read(fd, read_buf + read_buf_len, READ_BUF_SIZE - read_buf_len);

    if (n == 0) return false;

    if (n < 0) {
        LOG_E("Some error occurred while reading from %s (%d)", s_addr, fd);
        return false;
    }

    read_buf_len += n;

    return true;
}


bool HttpClient::Send()
{
    // hexdump((uint8_t *)write_buf, write_buf_len);
    int n = write(fd, write_buf, write_buf_len);

    if (n <= 0) {
        LOG_E("Some error occurred while writing to %s (%d)", s_addr, fd);
        return false;
    }

    return true;
}


void HttpClient::TruncateReadBuf()
{
    read_buf_len = 0;
}


