#ifndef TCP_SESSION_H_SENTRY
#define TCP_SESSION_H_SENTRY

#define READ_BUF_SIZE 4096


class TcpSession
{
    int fd;

    char read_buf[READ_BUF_SIZE];
    int read_buf_len;

    char *write_buf;
    int write_buf_len;

    char *s_addr;

    bool want_to_close;

public:
    TcpSession();
    TcpSession(int, char *);
    ~TcpSession();

    bool Read();
    bool Send();

    int GetFd() const { return fd; }
    const char *GetSAddr() const { return s_addr; }
    char *GetReadBuf() const;
    bool GetWantToClose() const { return want_to_close; }

    void SetWriteBuf(char *, int);
    void SetWantToClose(bool b) { want_to_close = b; }

    void TruncateReadBuf();
};


#endif
