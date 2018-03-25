#include "http_response.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"


HttpResponse::HttpResponse()
{
    memset(this, 0, sizeof(*this));
}


HttpResponse::~HttpResponse()
{
    free(response);
    free(body);
}


ByteArray *HttpResponse::GetResponseByteArray(http_status_t _code)
{
    code = _code;
    finalised = false;

    return GetResponseByteArray();
}


ByteArray *HttpResponse::GetResponseByteArray()
{
    if (!finalised) {
        AddContentLengthHeader();
        CloseHeaders();

        AddBody();

        finalised = true;
    }

    return new ByteArray(response, response_len);
}


void HttpResponse::SetBody(ByteArray *_body)
{
    if (_body) {
        body = (char *)malloc(_body->size);
        memcpy(body, _body->data, _body->size);
        body_len = _body->size;
    }
}


void HttpResponse::SetBody(const char *_body)
{
    if (_body) {
        body_len = strlen(_body);
        body = (char *)malloc(body_len);
        memcpy(body, _body, body_len);
    }
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


#define HTTP_RESPONSE_ENTRY(INT, ENUM, STATUS, BODY) \
    case ENUM:                                       \
        s_code = STATUS;                             \
        break;                                       \

void HttpResponse::AddStatusLine()
{
    const char *s_code;

    switch(code) {
        HTTP_RESPONSE_GEN
    }

    int len = sizeof("HTTP/1.X \r\n") - 1 + strlen(s_code);

    // +1 because snprintf wants to write \0
    response = (char *)realloc(response, len + 1);
    int ret = snprintf(response, len + 1, "HTTP/1.%d %s\r\n", minor_version, s_code);
    assert(ret);

    response_len += len;
}

#undef HTTP_RESPONSE_ENTRY

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


void HttpResponse::AddLastModifiedHeader(time_t time)
{
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&time));
    AddHeader("Last-Modified", timebuf);
}


void HttpResponse::AddLocationHeader(const char *loc)
{
    AddHeader("Location", loc);
}


void HttpResponse::AddCookieHeader(const char *key, const char *value)
{
    int header_value_len = strlen(key) + sizeof "=" + strlen(value);
    char *header_value = (char *)malloc(header_value_len);

    strcpy(header_value, key);
    strcat(header_value, "=");
    strcat(header_value, value);

    AddHeader("Set-Cookie", header_value);
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


#define HTTP_RESPONSE_ENTRY(INT, ENUM, STATUS, BODY) \
    case ENUM:                                       \
        str = BODY;                                  \
        break;                                       \

void HttpResponse::AddDefaultBodies()
{
    const char *str = NULL;

    switch(code) {
        HTTP_RESPONSE_GEN
    default:
        ;
    }

    if (str) {
        ByteArray _str(str, strlen(str));
        SetBody(&_str);
    }
}

#undef HTTP_RESPONSE_ENTRY


void HttpResponse::Reset()
{
    this->~HttpResponse();
    memset(this, 0, sizeof(*this));
}
