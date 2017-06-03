#include <string.h>
#include <stdio.h>

#include "exceptions.h"


#define MSG "Got a bad file from user "
HttpBadFile::HttpBadFile(const char *user)
{
    int len = sizeof MSG + strlen(user);
    char *_err_msg = (char *)malloc(len);

    snprintf(_err_msg, len, MSG "%s", user);

    SetErrMsg(_err_msg);
    free(_err_msg);
}
#undef MSG
