#ifndef PH_EXCEPTIONS_H_SENTRY
#define PH_EXCEPTIONS_H_SENTRY

#include <string.h>
#include <stdlib.h>


class Exception
{
    char *err_msg;

public:
    Exception(): err_msg(NULL) {}
    Exception(const char *t) { err_msg = strdup(t); }
    virtual ~Exception() { free(err_msg); }

    virtual const char *GetErrMsg() const { return err_msg; }
    virtual void SetErrMsg(const char *t) { if (t) err_msg = strdup(t); }
};

class SystemEx: public Exception {

public:
    SystemEx() {};
    SystemEx(const char *t): Exception(t) {};
};

class NoSpace: public SystemEx {};

class SaveFileEx: public SystemEx {

public:
    SaveFileEx(const char *t): SystemEx(t) {}
};

class StatEx: public SystemEx {

public:
    StatEx(const char *t): SystemEx(t) {}
};

class UnknownWriteError: public SystemEx {};
class UnknownReadError: public SystemEx {};


class AuthEx: public Exception {

public:
    AuthEx(const char *t): Exception(t) {}
};

class GetUserBySessionEx: public AuthEx {

public:
    GetUserBySessionEx(const char *t): AuthEx(t) {}
};

class NewSessionEx: public AuthEx {

public:
    NewSessionEx(const char *t): AuthEx(t) {}
};

class DeleteSessionEx: public AuthEx {

public:
    DeleteSessionEx(const char *t): AuthEx(t) {}
};


class UserEx : public Exception {};


class HttpEx: public Exception {};

class HttpBadFile: public HttpEx {

public:
    HttpBadFile(const char *user);
};


class PhotohostingEx: public Exception {};


#endif
