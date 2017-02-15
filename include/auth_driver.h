#ifndef AUTH_DRIVER_H_SENTRY
#define AUTH_DRIVER_H_SENTRY


class AuthDriver {
public:
    virtual bool Check(const char *_login, const char *password) const = 0;
};


#endif
