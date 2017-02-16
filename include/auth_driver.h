#ifndef AUTH_DRIVER_H_SENTRY
#define AUTH_DRIVER_H_SENTRY


class AuthDriver {
public:
    virtual bool Check(const char *_login, const char *password) const = 0;
    virtual char *NewSession(const char *user) =  0;
    virtual const char *GetUserBySession(const char *sid) const = 0;
};


#endif
