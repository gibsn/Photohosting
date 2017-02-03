#ifndef MULTIPART_H_SENTRY
#define MULTIPART_H_SENTRY


#include "multipart_parser.h"


struct ByteArray;


class MultipartParser
{
    multipart_parser *m_parser;
    multipart_parser_settings m_callbacks;

    char *filename;
    ByteArray *body;

    static int ReadHeaderValue(multipart_parser *, const char *, size_t);
    static int ReadBody(multipart_parser *, const char *, size_t);

public:
    MultipartParser(const char *);
    ~MultipartParser();

    void Execute(const ByteArray &);

    ByteArray *GetBody() { return body; }
    char *GetFilename() { return filename; }
};
#endif
