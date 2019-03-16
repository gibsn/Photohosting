#include "http_client.h"

#include <stdint.h>
#include <stdlib.h>

#include "tcp_client.h"


HttpClient::HttpClient(struct sue_event_selector &selector)
    : state(http_client_idle),
    t_client(NULL)
{
    t_client = new TcpClient(selector);
}

HttpClient::~HttpClient()
{
    delete t_client;
}

void HttpClient::OnRead()
{
    switch(state) {
    case http_client_idle:
        break;
    case http_client_waiting_get_response:
        ProcessWaitingPostResponse();
        break;
    }
}

void HttpClient::OnWrite()
{
}

ApplicationLevelBridge *HttpClient::Create(TransportSessionBridge *t)
{
    return this;
}

void HttpClient::Close(ApplicationLevelBridge *h)
{
}

void HttpClient::Dial(const char *addr)
{
    // split addr to host and port
    // resolve domain name
    char *host;
    uint16_t port;
    t_client->Dial(host, port, this);
}

// TODO consider using some lib for params
void HttpClient::Post(const char *path, const char *params)
{
}

// TODO
void HttpClient::ProcessWaitingPostResponse()
{
}
