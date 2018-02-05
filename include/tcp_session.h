#ifndef TCP_SESSION_H_SENTRY
#define TCP_SESSION_H_SENTRY

#define READ_BUF_SIZE 4096

extern "C" {
#include "sue_base.h"
}

#include "app_layer_driver.h"
#include "common.h"
#include "select_loop_driver.h"

typedef void (*sue_fd_handler_cb_t)(struct sue_fd_handler*, int, int, int);


class TcpServer;
class AppLayerDriver;

class TcpSession
{
    sue_fd_handler fd_h;

    bool active;
    char *s_addr;

    AppLayerDriver *session_driver;
    TcpServer *tcp_server;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    char *write_buf;
    int write_buf_len;
    int write_buf_offset;

    bool want_to_close;

    void TruncateReadBuf();
    void TruncateWriteBuf();

public:
    TcpSession();
    TcpSession(int fd, char *s_addr, TcpServer *tcp_server);
    ~TcpSession();

    bool ReadToBuf();
    bool Flush();

    bool ProcessRequest();
    void Close();

    void InitFdHandler(sue_fd_handler_cb_t cb) {
        fd_h.userdata = this;
        fd_h.handle_fd_event = cb;
    }

    TcpServer *GetTcpServer() { return tcp_server; }
    int GetFd() const { return fd_h.fd; }
    const char *GetSAddr() const { return s_addr; }
    ByteArray *GetReadBuf();
    bool GetWantToClose() const { return want_to_close; }
    sue_fd_handler *GetFdHandler() { return &fd_h; }

    void Send(ByteArray *);
    void SetWantToClose(bool b) { want_to_close = b; }
    void SetSessionDriver(AppLayerDriver *_sd) { session_driver = _sd; }
};


#endif
