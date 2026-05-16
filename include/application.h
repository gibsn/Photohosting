#ifndef APPLICATION_H_SENTRY
#define APPLICATION_H_SENTRY


class TransportSessionBridge;
class TransportSessionManagerBridge;

class ApplicationLevelBridge
{
public:
    virtual ~ApplicationLevelBridge() {};

    virtual void OnRead() = 0;
    virtual void OnWrite() = 0;
};

class ApplicationLevelSessionManagerBridge
{
public:
    virtual ~ApplicationLevelSessionManagerBridge() {};

    virtual ApplicationLevelBridge *Create(TransportSessionBridge *t) = 0;
    virtual void Close(ApplicationLevelBridge *h) = 0;
};

#endif
