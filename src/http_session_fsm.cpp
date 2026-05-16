#include "http_session.h"

#include <ctype.h>
#include <string.h>

#include "log.h"
#include "tcp_session.h"


typedef enum {
    invalid_request = -1,
    incomplete_headers = -2,
} pico_request_parser_result_t;


void HttpSession::Routine()
{
    while (true) {
        switch(state) {
        case(state_idle):
            ProcessStateIdle();
            break;
        case(state_waiting_for_headers):
            ProcessStateWaitingForHeaders();
            break;
        case(state_processing_headers):
            ProcessStateProcessingHeaders();
            break;
        case(state_waiting_for_body):
            ProcessStateWaitingForBody();
            break;
        case(state_processing_request):
            ProcessStateProcessingRequest();
            break;
        case(state_responding):
            ProcessStateResponding();
            break;
        case(state_shutting_down):
            ProcessStateShuttingDown();
            break;
        }

        if (should_wait_for_more_data) {
            return;
        }
    }
}


void HttpSession::ProcessStateIdle()
{
    state = state_waiting_for_headers;
}


void HttpSession::ProcessStateWaitingForHeaders()
{
    // pico does no allocations itself so underlying headers buf should be
    // saved since read_buf is truncated
    char *buf = strndup(read_buf.data, read_buf.size);
    uint32_t buf_len = read_buf.size;

    int res = phr_parse_request(
        buf,
        buf_len,
        (const char**)&request.method,
        &request.method_len,
        (const char **)&request.path,
        &request.path_len,
        &request.minor_version,
        request.headers,
        &request.n_headers,
        last_headers_len
    );

    if (res > 0) {
        request.headers_raw = buf;
        request.headers_len = res;
        read_buf.Truncate(request.headers_len);

        state = state_processing_headers;

        return;
    }

    free(buf);

    if (res == incomplete_headers) {
        LOG_D("http: got incomplete headers");

        request.Reset();
        last_headers_len = read_buf.size;
        should_wait_for_more_data = true;

        return;
    }

    LOG_D("http: invalid headers");

    InitHttpResponse(http_bad_request);
    SetWantToClose();

    state = state_responding;
}


#define CMP_HEADER(str) !memcmp(headers[i].name, str, headers[i].name_len)

void HttpSession::ProcessStateProcessingHeaders()
{
    LOG_D("http: Method: %.*s", int(request.method_len), request.method);
    LOG_D("http: Path: %.*s", int(request.path_len), request.path);

    struct phr_header *headers = request.headers;
    for (int i = 0; i < request.n_headers; ++i) {
        LOG_D("http: %.*s: %.*s",
                int(headers[i].name_len), headers[i].name,
                int(headers[i].value_len), headers[i].value
        );

        if (CMP_HEADER("Connection")) {
            keep_alive =
                headers[i].value_len > 0 &&
                tolower(headers[i].value[0]) == 'k';
        } else if (CMP_HEADER("Content-Length")) {
            request.body_len = strtol(headers[i].value, NULL, 10);
        } else if (CMP_HEADER("Cookie")) {
            request.ParseCookie(headers[i].value, headers[i].value_len);
        } else if (CMP_HEADER("If-Modified-Since")) {
            request.if_modified_since = strndup(headers[i].value, headers[i].value_len);
        }
    }

    state = state_waiting_for_body;
}

#undef CMP_HEADER


void HttpSession::ProcessStateWaitingForBody()
{
    int body_bytes_pending = request.body_len - read_buf.size;
    if (body_bytes_pending) {
        LOG_D("http: missing %d body bytes, waiting", body_bytes_pending);
        should_wait_for_more_data = true;
        return;
    }

    // TODO no need to allocate and copy
    request.body = (char *)malloc(request.body_len);
    memcpy(request.body, read_buf.data, request.body_len);

    read_buf.Truncate(request.body_len);

    state = state_processing_request;
}


#define METHOD_IS(str)\
    request.method_len == strlen(str) && \
    !strncmp(request.method, str, MIN(request.method_len, strlen(str)))

void HttpSession::ProcessStateProcessingRequest()
{
    if (!ValidateLocation(strndup(request.path, request.path_len))) {
        InitHttpResponse(http_bad_request);
    } else if (METHOD_IS("GET")) {
        ProcessGetRequest();
    } else if (METHOD_IS("POST")) {
        ProcessPostRequest();
    } else {
        InitHttpResponse(http_not_found);
    }

    state = state_responding;
}


void HttpSession::ProcessStateResponding()
{
    Respond();

    request.Reset();
    response.Reset();

    if (!keep_alive) {
        state = state_shutting_down;
        return;
    }

    should_wait_for_more_data = true;
    state = state_idle;
}


void HttpSession::ProcessStateShuttingDown()
{
    t_session->WantToClose();
    should_wait_for_more_data = true;
}

#undef METHOD_IS
