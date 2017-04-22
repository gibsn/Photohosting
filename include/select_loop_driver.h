#ifndef SELECT_LOOP_H_SENTRY
#define SELECT_LOOP_H_SENTRY


class SelectLoopDriver
{
public:
    virtual ~SelectLoopDriver() {};

    virtual void RequestWriteForFd(int fd) = 0;
};


#endif
