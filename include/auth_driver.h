#ifndef AUTH_DRIVER_H_SENTRY
#define AUTH_DRIVER_H_SENTRY


class AuthDriver {
public:
    virtual ~AuthDriver() {};

    // throws CheckEx in case of error
    virtual bool Check(const char *_login, const char *password) const = 0;

    // throws NewSessionEx in case of error
    virtual char *NewSession(const char *user) = 0;

    // throws DeleteSessionEx in case of error
    virtual void DeleteSession(const char *sid) = 0;

    // throws GetUserBySessionEx in case of error
    virtual char *GetUserBySession(const char *sid) const = 0;
};


#endif
