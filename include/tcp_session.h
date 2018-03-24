#ifndef TCP_SESSION_H_SENTRY
#define TCP_SESSION_H_SENTRY

#define READ_BUF_SIZE 4096
#define IDLE_TIMEOUT 60 //sec

extern "C" {
#include "sue_base.h"
}

#include "app_layer_driver.h"
#include "common.h"
#include "select_loop_driver.h"

typedef void (*sue_fd_handler_cb_t)(struct sue_fd_handler*, int, int, int);
typedef void (*sue_tout_handler_cb_t)(struct sue_timeout_handler*);

class TcpServer;
class AppLayerDriver;

class TcpSession
{
    sue_fd_handler fd_h;
    sue_timeout_handler tout_h;

    char *s_addr;

    AppLayerDriver *app_layer_session;
    TcpServer *tcp_server;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    char *write_buf;
    int write_buf_len;
    int write_buf_offset;

    bool want_to_close;

    void ResetReadBuf();
    void ResetWriteBuf();

public:
    TcpSession(
        char *_s_addr,
        TcpServer *_tcp_server,
        int _fd,
        sue_fd_handler_cb_t cb,
        sue_tout_handler_cb_t tout_cb
    );

    ~TcpSession();

    bool ReadToBuf();
    bool Flush();

    void ProcessRequest();
    void Close();

    TcpServer *GetTcpServer() { return tcp_server; }
    int GetFd() const { return fd_h.fd; }
    const char *GetSAddr() const { return s_addr; }
    ByteArray *GetReadBuf();
    bool GetWantToClose() const { return want_to_close; }
    AppLayerDriver *GetAppLayerSession() { return app_layer_session; }

    sue_fd_handler *GetFdHandler() { return &fd_h; }
    sue_timeout_handler *GetToutHandler() { return &tout_h; }

    void SetWantToClose(bool b) { want_to_close = b; }
    void SetAppLayerSession(AppLayerDriver *session) { app_layer_session = session; }

    void InitFdHandler(sue_event_selector *selector);

    void InitIdleTout(sue_event_selector *selector);
    void ResetIdleTout(sue_event_selector *selector);

    void Send(ByteArray *);
};


#endif
