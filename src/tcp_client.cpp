#include "tcp_client.h"

#include <assert.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common.h"
#include "exceptions.h"
#include "log.h"
#include "tcp_session.h"


TcpClient::TcpClient(sue_event_selector &selector)
    : selector(selector),
    tcp_session(NULL)
{
}

TcpClient::~TcpClient()
{
    delete tcp_session;
}

void TcpClient::OnRead(TcpSession *session)
{
    if (!session->IsActive()) {
        return;
    }

    session->OnRead();

    if (session->ShouldClose()) {
        delete session;
    }
}

void TcpClient::OnWrite(TcpSession *session)
{
    if (!session->IsActive()) {
        return;
    }

    session->OnWrite();

    if (session->ShouldClose()) {
        delete session;
    }
}

void TcpClient::OnX(TcpSession *session)
{
}

void TcpClient::OnTimeout(TcpSession *session)
{
    LOG_I("tcp: closing idle connection [addr=%s; fd=%d]", session->GetAddr(), session->GetFd());

    delete session;
}

char *TcpClient::AddrFromHostPort(const char *host, uint16_t port)
{
    char s_port[6];
    snprintf(s_port, sizeof(s_port), "%u", port);

    uint16_t s_addr_len = strlen(host) + 1 + strlen(s_port) + 1;
    char *s_addr = (char *)calloc(s_addr_len, sizeof(*s_addr));
    if (!s_addr) {
        return NULL;
    }

    strcat(s_addr, host);
    strcat(s_addr, ":");
    strcat(s_addr, s_port);

    return s_addr;
}


void TcpClient::Dial(const char *host, uint16_t port, ApplicationLevelSessionManagerBridge *asb)
{
	int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw ConnectionError(strerror(errno));
    }

    int args = fcntl(fd, F_GETFD, 0);
    assert(args != -1);
    assert(fcntl(fd, F_SETFL, args | O_NONBLOCK) != -1);

	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(port);

	int ret = inet_pton(AF_INET, host, &addr.sin_addr);
    if (ret == -1) {
        throw ConnectionError(strerror(errno));
    } else if (ret == 0) {
        throw ConnectionError("host is not parseable");
    }

    // TODO connect timeout
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) && errno != EINPROGRESS) {
        throw ConnectionError(strerror(errno));
	}

    char *s_addr = AddrFromHostPort(host, port);
    tcp_session = new TcpSession(s_addr, fd, this, selector);
}

// TODO
void TcpClient::Shutdown()
{
}

void TcpClient::Write(ByteArray *buf)
{
    tcp_session->Write(buf);
}

ByteArray *TcpClient::Read()
{
    return tcp_session->Read();
}
