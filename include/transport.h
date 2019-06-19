#ifndef TRANSPORT_H_SENTRY
#define TRANSPORT_H_SENTRY

#include <stdint.h>


struct ByteArray;

class ApplicationLevelBridge;
class ApplicationLevelSessionManagerBridge;


class TransportSessionBridge {
public:
    virtual ~TransportSessionBridge() {};

    virtual const char *GetAddr() = 0;

    virtual ByteArray *Read() = 0;
    virtual void Write(ByteArray *buf) = 0;
    virtual void WantToClose() = 0;
};

class TransportServerBridge {
public:
    virtual ~TransportServerBridge() {};

    virtual void Shutdown() = 0;
};

class TransportClientBridge {
public:
    virtual ~TransportClientBridge() {};

    virtual void Dial(const char *addr, uint16_t port,
                      ApplicationLevelSessionManagerBridge *asb
    ) = 0;

    virtual void Shutdown() = 0;

    virtual ByteArray *Read() = 0;
    virtual void Write(ByteArray *buf) = 0;
};

#endif
