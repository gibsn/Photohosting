#include "http_response.h"

#include <assert.h>
#include "common.h"
#include <stdlib.h>
#include <string.h>

#include "log.h"


HttpResponse::HttpResponse(
    http_status_t _code,
    ByteArray *_body = NULL,
    int _minor_version = 0,
    bool _keep_alive = false
)
    : response(NULL),
    response_len(0),
    minor_version(_minor_version),
    code(_code),
    keep_alive(_keep_alive),
    body(NULL),
    body_len(0),
    finalised(false)
{
    if (_body) {
        body = (char *)malloc(_body->size);
        memcpy(body, _body->data, _body->size);
        body_len = _body->size;
    }
}


HttpResponse::~HttpResponse()
{
    if (response) free(response);
    if (body) free(body);
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
        AddStatusLine();

        AddDateHeader();
        AddServerHeader();
        AddConnectionHeader(keep_alive);

        AddContentLengthHeader();

        CloseHeaders();
        AddBody();

        response = (char *)realloc(response, response_len + 1);
        response[response_len] = '\0';

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
    case http_continue:
        s_code = "100 CONTINUE";
        break;
    case http_ok:
        s_code = "200 OK";
        break;
    case http_created:
        s_code = "201 CREATED";
        break;
    case http_no_content:
        s_code = "204 NO CONTENT";
        break;
    case http_permanent_redirect:
        s_code = "308 PERMANENT REDIRECT";
        break;
    case http_bad_request:
        s_code = "400 BAD REQUEST";
        break;
    case http_not_found:
        s_code = "404 NOT FOUND";
        break;
    case http_internal_error:
        s_code = "500 INTERNAL SERVER ERROR";
        break;
    case http_not_implemented:
        s_code = "501 NOT IMPLEMENTED";
        break;
    }

    int len = sizeof("HTTP/1.X \r\n") - 1 + strlen(s_code);

    // +1 because snprintf wants to write \0
    response = (char *)realloc(response, len + 1);
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
