#include "http_request.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"


HttpRequest::HttpRequest()
    : method(NULL),
    method_len(0),
    path(NULL),
    path_len(0),
    headers_len(0),
    n_headers(100),
    body(NULL),
    body_len(0)
{
}


HttpRequest::~HttpRequest()
{
    if (body) free(body);
}


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

char *HttpRequest::GetMultipartBondary() const
{
    for (int i = 0; i < n_headers; ++i) {
        if (CMP_HEADER("Content-Type")) {
            char *boundary = NULL;
            char *end;

            char *value = strndup(headers[i].value, headers[i].value_len);
            char *start = strstr(value, "boundary=");
            if (!start) goto fin;

            start += sizeof "boundary=" - 1;

            end = strchr(start, ';');

            if (end) {
                boundary = strndup(start, end - start);
            } else {
                boundary = strdup(start);
            }

fin:
            free(value);

            return boundary;
        }
    }

    return NULL;
}

#undef CMP_HEADER
