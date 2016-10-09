#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "http_server.h"
#include "log.h"


int main(int argc, char **argv)
{
    Config cfg;
    my_pid = getpid();

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;
    cfg.Check();
    LOG_I("Initialised config");

    LOG_I("Starting server");
    HttpServer server;
    server.SetArgs(cfg);
    server.Init();
    server.ListenAndServe();

    return 0;
}
