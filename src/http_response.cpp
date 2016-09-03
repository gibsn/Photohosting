#include "http_response.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"


HttpResponse::HttpResponse()
    : headers_list(NULL),
    n_headers(0),
    body(NULL),
    body_len(0)
{
}


HttpResponse::~HttpResponse()
{
    HeadersList *p = headers_list;

    while(p) {
        free((char *)p->header.name);
        free((char *)p->header.value);

        HeadersList *tmp = p;
        p = p->next;

        delete tmp;
    }
}


void HttpResponse::AddHeader(const char *name, const char *value)
{
    HeadersList *new_header = new HeadersList;

    new_header->header.name = strdup(name);
    new_header->header.name_len = strlen(name);
    new_header->header.value = strdup(value);
    new_header->header.value_len = strlen(value);

    new_header->next = headers_list;
    headers_list = new_header;

    n_headers++;
}


void HttpResponse::AddDateHeader()
{
    char timebuf[32];
    time_t now = time(0);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    AddHeader("Date", timebuf);
}


char *HttpResponse::GenerateResponse(int &len)
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

    len = 11 + strlen(s_code);
    // +1 cause snprintf wants to write \0
    char *response = (char *)malloc(sizeof(char) * (len + 1));
    snprintf(response, len + 1, "HTTP/1.%d %s\r\n", minor_version, s_code);

    HeadersList *header = headers_list;
    char *s_header;
    int s_header_len;
    while (header) {
        s_header = HeaderToStr(header->header);
        s_header_len = strlen(s_header);

        LOG_E("%s", s_header);
        response = (char *)realloc(response, len + s_header_len);
        memcpy(response + len, s_header, s_header_len);

        len += s_header_len;

        free(s_header);
        header = header->next;
    }

    response = (char *)realloc(response, len + 2);
    memcpy(response + len, "\r\n", 2);

    len += 2;

    // hexdump((uint8_t *)response, len);

    return response;

#undef HEAD
}


char *HttpResponse::HeaderToStr(const phr_header &header)
{
    int len = header.name_len + header.value_len + 5;
    char *buf = (char *)malloc(sizeof(char) * len);

    memcpy(buf, header.name, header.name_len);
    memcpy(buf + header.name_len, ": ", 2);
    memcpy(buf + header.name_len + 2, header.value, header.value_len);
    memcpy(buf + header.name_len + 2 + header.value_len, "\r\n", 2);

    buf[len - 1] = '\0';

    return buf;
}

