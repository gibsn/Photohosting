#include "http_response.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"


HttpResponse::HttpResponse()
    : response(NULL),
    response_len(0),
    body(NULL),
    body_len(0)
{
}


HttpResponse::~HttpResponse()
{
    if (response) free(response);
    if (body) free(body);
}


void HttpResponse::AddHeader(const char *name, const char *value)
{
    int len = strlen(name) + 2 + strlen(value) + 2;
    response = (char *)realloc(response, response_len + len);

    memcpy(response + response_len, name, strlen(name));
    response_len += strlen(name);

    memcpy(response + response_len, ": ", 2);
    response_len += 2;

    memcpy(response + response_len, value, strlen(value));
    response_len += strlen(value);

    memcpy(response + response_len, "\r\n", 2);
    response_len += 2;
}


void HttpResponse::AddServerHeader()
{
    AddHeader("Server", "gibsnHttpServer");
}


void HttpResponse::AddDateHeader()
{
    char timebuf[32];
    time_t now = time(0);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    AddHeader("Date", timebuf);
}


void HttpResponse::AddStatusLine()
{
    const char *s_code;

    switch(code) {
    case http_ok:
        s_code = "200 OK";
        break;
    case http_bad_request:
        s_code = "400 BAD REQUEST";
        break;
    case http_not_found:
        s_code = "404 NOT FOUND";
        break;
    default:
        ;
    }

    int len = 11 + strlen(s_code);
    // +1 cause snprintf wants to write \0
    response = (char *)realloc(response, sizeof(char) * (len + 1));
    int ret =
        snprintf(response, len + 1, "HTTP/1.%d %s\r\n", minor_version, s_code);
    assert(ret);

    response_len += len;
}


void HttpResponse::AddConnectionHeader(bool keep_alive)
{
    AddHeader( "Connection", keep_alive? "keep-alive" : "close");
}

void HttpResponse::AddContentLengthHeader()
{
    char buf[32];
    int ret = snprintf(buf, sizeof(buf), "%d", body_len);
    assert(ret);

    AddHeader("Content-Length", buf);
}


void HttpResponse::CloseHeaders()
{
    response = (char *)realloc(response, response_len + strlen("\r\n"));
    memcpy(response + response_len, "\r\n", strlen("\r\n"));
    response_len += strlen("\r\n");
}


void HttpResponse::AddBody()
{
   if (body_len) {
       response = (char *)realloc(response, response_len + body_len);
       memcpy(response + response_len, body, body_len);
       response_len += body_len;
    }
}
