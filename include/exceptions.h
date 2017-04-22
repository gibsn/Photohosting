#ifndef PH_EXCEPTIONS_H_SENTRY
#define PH_EXCEPTIONS_H_SENTRY

#include <string.h>
#include <stdlib.h>


class PhotohostingEx
{

protected:
    char *text;

public:
    PhotohostingEx(): text(NULL) {}
    PhotohostingEx(const char *t) { text = strdup(t); }
    virtual ~PhotohostingEx() { if (text) free(text); }

    virtual const char *GetErrMsg() const { return text; }
};

class SystemEx: public PhotohostingEx {};

class NoSpace: public SystemEx {};
class UnknownWriteError: public SystemEx {};

class UserEx : public PhotohostingEx {};

class HttpEx: public PhotohostingEx {};

class HttpBadFile: public HttpEx {

public:
    HttpBadFile(const char *user);
};


#endif
