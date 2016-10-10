#ifndef HTTP_CLIENT_H_SENTRY
#define HTTP_CLIENT_H_SENTRY

#include "http_request.h"
#include "http_response.h"

#define READ_BUF_SIZE 4096


struct HttpClient {
    int fd;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    char *write_buf;
    int write_buf_len;

    char *s_addr;

    HttpRequest request;
    HttpResponse response;

    HttpClient();
    ~HttpClient();

    bool Read();
    bool Send();

    void TruncateReadBuf();
};

#endif
