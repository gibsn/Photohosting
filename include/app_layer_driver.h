#ifndef APP_LAYER_DRIVER_H_SENTRY
#define APP_LAYER_DRIVER_H_SENTRY

class AppLayerDriver
{
public:
    virtual ~AppLayerDriver() {};

    virtual void OnRead() = 0;
    virtual void OnWrite() = 0;
};


#endif
