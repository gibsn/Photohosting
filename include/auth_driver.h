#ifndef AUTH_DRIVER_H_SENTRY
#define AUTH_DRIVER_H_SENTRY


class AuthDriver {
public:
    virtual ~AuthDriver() {};

    // throw CheckEx in case of error
    virtual bool Check(const char *_login, const char *password) const = 0;

    // throw NewSessionEx in case of error
    virtual char *NewSession(const char *user) = 0;

    // throw DeleteSessionEx in case of error
    virtual void DeleteSession(const char *sid) = 0;

    // throw GetUserBySessionEx in case of error
    virtual char *GetUserBySession(const char *sid) const = 0;
};


#endif
