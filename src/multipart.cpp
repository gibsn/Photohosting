#include "multipart.h"

#include <stdio.h>
#include <string.h>

#include "common.h"


int MultipartParser::ReadHeaderValue(multipart_parser *p, const char *at, size_t length)
{
    MultipartParser *me = (MultipartParser*)multipart_parser_get_data(p);

    char *value = strndup(at, length);
    char *end;
    char *start = strstr(value, "filename=");
    if (!start) goto fin;

    start += sizeof "filename=";
    end = strchr(start, ';');

    if (end) {
        me->filename = strndup(start, end - start - 1);
    } else {
        me->filename = strndup(start, length - (start - value) - 1);
    }

fin:
    free(value);

    return 0;
}


int MultipartParser::ReadBody(multipart_parser *p, const char *at, size_t length)
{
    MultipartParser *me = (MultipartParser*)multipart_parser_get_data(p);
    int boundary_len = strlen(me->boundary) - 2;
    int min_size = length < boundary_len? length: boundary_len;

    if (0 != strncmp(me->boundary + 2, at, min_size)) {
        ByteArray *part_body = new ByteArray(at, length);
        me->body->Append(part_body);
        delete part_body;
    }

    return 0;
}


MultipartParser::MultipartParser(const char *_boundary)
    : boundary(_boundary),
    filename(NULL),
    body(NULL)
{
    memset(&m_callbacks, 0, sizeof(multipart_parser_settings));
    m_callbacks.on_header_value = ReadHeaderValue;
    m_callbacks.on_part_data = ReadBody;

    body = new ByteArray();

    m_parser = multipart_parser_init(boundary, &m_callbacks);
    multipart_parser_set_data(m_parser, this);
}


MultipartParser::~MultipartParser()
{
    if (filename) free(filename);
    if (body) delete body;

    multipart_parser_free(m_parser);
}


void MultipartParser::Execute(const ByteArray &_body)
{
    int n = multipart_parser_execute(m_parser, _body.data + 2, _body.size - 2);
}
