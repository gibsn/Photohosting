#include <string.h>
#include <stdio.h>

#include "exceptions.h"


HttpBadFile::HttpBadFile(const char *user)
{
    static const char *msg = "Got a bad file from user ";
    text = (char *)malloc(sizeof msg + strlen(user));
    strcpy(text, msg);
    strcat(text, user);
}
