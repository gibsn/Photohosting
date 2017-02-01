#include "http_request.h"

#include <stdlib.h>


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
