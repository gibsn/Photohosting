#ifndef APP_LAYER_DRIVER_H_SENTRY
#define APP_LAYER_DRIVER_H_SENTRY

class AppLayerDriver
{
public:
    virtual ~AppLayerDriver() {};

    virtual bool ProcessRequest() = 0;
    virtual void Close() = 0;
};


#endif
