#include "http_request.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"


HttpRequest::HttpRequest()
    : method(NULL),
    method_len(0),
    path(NULL),
    path_len(0),
    headers_raw(NULL),
    headers_len(0),
    n_headers(MAX_HTTP_HEADERS),
    sid(NULL),
    if_modified_since(NULL),
    body(NULL),
    body_len(0)
{
}


HttpRequest::~HttpRequest()
{
    free(headers_raw);
    free(sid);
    free(if_modified_since);
    free(body);
}


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

char *HttpRequest::GetMultipartBondary() const
{
    for (int i = 0; i < n_headers; ++i) {
        if (CMP_HEADER("Content-Type")) {
            char *value = strndup(headers[i].value, headers[i].value_len);
            char *start = strstr(value, "boundary=");
            free(value);
            if (!start) {
                return NULL;
            }

            start += sizeof "boundary=" - 1;

            char *end = strchr(start, ';');
            if (end) {
                return strndup(start, end - start);
            }

            return strdup(start);
        }
    }

    return NULL;
}

#undef CMP_HEADER


//TODO: parse the whole cookie
void HttpRequest::ParseCookie(const char *value, int len)
{
    char *value_copy = strndup(value, len);
    char *start = strstr(value_copy, "sid=");

    if (!start) {
        free(value_copy);
        return;
    }

    start += sizeof "sid=" - 2;
    char *save_start = start + 1;

    while (isalnum(*++start));

    *start = '\0';

    sid = strdup(save_start);

    free(value_copy);
}


void HttpRequest::Reset()
{
    this->~HttpRequest();
    memset(this, 0, sizeof(*this));
    n_headers = MAX_HTTP_HEADERS;
}
