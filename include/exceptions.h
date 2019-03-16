#ifndef PH_EXCEPTIONS_H_SENTRY
#define PH_EXCEPTIONS_H_SENTRY

#include <string.h>
#include <stdlib.h>


class Exception
{
    char *err_msg;

public:
    Exception(): err_msg(NULL) {}
    Exception(const char *t): err_msg(NULL) { if (t) err_msg = strdup(t); }
    virtual ~Exception() { free(err_msg); }

    virtual const char *GetErrMsg() const { return err_msg; }
    virtual void SetErrMsg(const char *t) { if (t) err_msg = strdup(t); }
};

class SystemEx: public Exception {

public:
    SystemEx() {};
    SystemEx(const char *t): Exception(t) {};
};

class NoSpace: public SystemEx {

public:
    NoSpace(const char *t): SystemEx(t) {}
};

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

class ConnectionError: public SystemEx {

public:
    ConnectionError(const char *t): SystemEx(t) {}
    //
    // virtual const char *GetErrMsg() const {
    //     const char *base_msg = SystemEx::GetErrMsg();
    //     const char *error_type_msg = "connection error: ";
    //     uint16_t concrete_msg_len = strlen(error_type_msg) + strlen(base_msg) + 1;
    //     char *concrete_msg = (char *)malloc(sizeof(*concrete_msg)*concrete_msg_len);
    //     concrete_msg[0] = 0;
    //     strcat(concrete_msg, error_type_msg);
    //     return strcat(concrete_msg, base_msg);
    // }
};


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


class UserEx: public Exception {

public:
    UserEx(const char *t): Exception(t) {}
};

class BadArchive: public UserEx {

public:
    BadArchive(const char *t): UserEx(t) {}
};

class BadImage: public UserEx {

public:
    BadImage(const char *t): UserEx(t) {}
};

class HttpBadPostBody: public UserEx {

public:
    HttpBadPostBody(const char *t): UserEx(t) {};
};

class HttpBadFile: public UserEx {

public:
    HttpBadFile(const char *t): UserEx(t) {};
};

class HttpBadPageTitle: public UserEx {

public:
    HttpBadPageTitle(const char *t): UserEx(t) {};
};


class PhotohostingEx: public Exception {

public:
    PhotohostingEx(const char *t): Exception(t) {}
};

class GetUsersEx: public PhotohostingEx {

public:
    GetUsersEx(const char *t): PhotohostingEx(t) {}
};

class GetAlbumTitlesEx: public PhotohostingEx {

public:
    GetAlbumTitlesEx(const char *t): PhotohostingEx(t) {}
};

class UserNotFound: public PhotohostingEx {

public:
    UserNotFound(const char *t): PhotohostingEx(t) {}
};

#endif
