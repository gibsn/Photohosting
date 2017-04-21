#ifndef TCP_SESSION_H_SENTRY
#define TCP_SESSION_H_SENTRY

#define READ_BUF_SIZE 4096


#include "app_layer_driver.h"
#include "common.h"
#include "select_loop_driver.h"


class TcpServer;
class AppLayerDriver;

class TcpSession
{
    bool active;
    int fd;

    SelectLoopDriver *select_driver;
    AppLayerDriver *session_driver;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    char *write_buf;
    int write_buf_len;

    char *s_addr;

    bool want_to_close;

public:
    TcpSession();
    TcpSession(int, char *, SelectLoopDriver *);
    ~TcpSession();

    bool ReadToBuf();
    bool Flush();

    bool ProcessRequest();
    void Close();

    int GetFd() const { return fd; }
    const char *GetSAddr() const { return s_addr; }
    ByteArray *GetReadBuf() const;
    bool GetWantToClose() const { return want_to_close; }

    void Send(ByteArray *);
    void SetWantToClose(bool b) { want_to_close = b; }
    void SetSessionDriver(AppLayerDriver *_sd) { session_driver = _sd; }

    void TruncateReadBuf();
};


#endif
