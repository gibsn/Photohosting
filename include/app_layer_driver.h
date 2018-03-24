#ifndef APP_LAYER_DRIVER_H_SENTRY
#define APP_LAYER_DRIVER_H_SENTRY

class AppLayerDriver
{
public:
    virtual ~AppLayerDriver() {};

    virtual void ProcessRequest() = 0;
};


#endif
