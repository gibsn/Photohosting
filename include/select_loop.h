#ifndef SELECT_LOOP_H_SENTRY
#define SELECT_LOOP_H_SENTRY


// Interface for interacting with the select loop
class SelectLoopDriver
{
public:
    virtual ~SelectLoopDriver() {};

    virtual void SetWantToWrite(int) = 0;
    // virtual void DeleteSession(int) = 0;
};


#endif
