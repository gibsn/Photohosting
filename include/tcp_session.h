#ifndef TCP_SESSION_H_SENTRY
#define TCP_SESSION_H_SENTRY

#define READ_BUF_SIZE 4096
#define IDLE_TIMEOUT 60 //sec

extern "C" {
#include "sue_base.h"
}

#include "application.h"
#include "common.h"
#include "transport.h"

typedef void (*sue_fd_handler_cb_t)(struct sue_fd_handler*, int, int, int);
typedef void (*sue_tout_handler_cb_t)(struct sue_timeout_handler*);

class TcpServer;
class TcpSession;

class TcpSessionManagerBridge {
public:
    virtual ~TcpSessionManagerBridge() {};

    virtual void OnRead(TcpSession *session) = 0;
    virtual void OnWrite(TcpSession *session) = 0;
    virtual void OnX(TcpSession *session) = 0;
    virtual void OnTimeout(TcpSession *session) = 0;
};

struct SueTcpSessionHandlerUserData {
    TcpSession *session;
    TcpSessionManagerBridge *session_manager;
};


class TcpSession: public TransportSessionBridge
{
    sue_event_selector &selector;
    sue_fd_handler fd_h;
    sue_timeout_handler tout_h;
    SueTcpSessionHandlerUserData userdata;

    ApplicationLevelBridge *application_level_handler;

    char *s_addr;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    // TODO change to bytearray
    char *write_buf;
    int write_buf_len;
    int write_buf_offset;

    bool want_to_close;


    static void FdHandlerCb(sue_fd_handler *fd_h, int r, int w, int x);
    void FdHandler(int r, int w, int x);

    static void TimeoutHandlerCb(sue_timeout_handler *tout_h);
    void TimeoutHandler();

    void ResetIdleTimeout();

    bool ReadToBuf();
    void ResetReadBuf();
    bool Flush();
    void ResetWriteBuf();

public:
    TcpSession(
        char *_s_addr,
        int _fd,
        TcpSessionManagerBridge *session_manager,
        sue_event_selector &selector
    );

    ~TcpSession();

    void OnRead();
    void OnWrite();

    void WantToClose() { want_to_close = true; }
    bool IsActive() const { return !want_to_close; }
    bool ShouldClose() const { return want_to_close; }
    void Close();

    int GetFd() const { return fd_h.fd; }
    const char *GetAddr() { return s_addr; }

    ApplicationLevelBridge *GetApplicationLevelHandler() { return application_level_handler; }
    void SetApplicationLevelHandler(ApplicationLevelBridge *h) {
        application_level_handler = h;
    }

    ByteArray *Read();
    void Write(ByteArray *);
};


#endif
