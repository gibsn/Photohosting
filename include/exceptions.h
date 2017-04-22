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

class SystemEx: public PhotohostingEx {

public:
    SystemEx() {};
    SystemEx(const char *t): PhotohostingEx(t) {};
};

class NoSpace: public SystemEx {};

class UnknownWriteError: public SystemEx {

public:
    UnknownWriteError(const char *t): SystemEx(t) {};
};

class UnknownReadError: public SystemEx {

public:
    UnknownReadError(const char *t): SystemEx(t) {};
};

class UserEx : public PhotohostingEx {};

class HttpEx: public PhotohostingEx {};

class HttpBadFile: public HttpEx {

public:
    HttpBadFile(const char *user);
};


#endif
