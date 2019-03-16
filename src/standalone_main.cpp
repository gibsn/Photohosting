#include <stdio.h>
#include <stdlib.h>

#include "auth.h"
#include "cfg.h"
#include "common.h"
#include "exceptions.h"
#include "http_server.h"
#include "log.h"
#include "photohosting.h"
#include "tcp_server.h"

#include "tcp_client.h"

int main(int argc, char **argv)
{
    Config cfg;

    if (!process_cmd_arguments(argc, argv, cfg)) {
        return EXIT_FAILURE;
    }

    if (!cfg.Parse(cfg.path_to_cfg)) {
        return EXIT_FAILURE;
    }

    init_logger(cfg);

    LOG_I("main: initialised logging");
    LOG_I("main: initialised config");

    Auth auth;
    if (!auth.Init(cfg.path_to_pwd, cfg.path_to_tokens)) {
        return EXIT_FAILURE;
    }

    LOG_I("main: initialised auth");

    Photohosting photohosting(cfg, &auth);
    if (!photohosting.Init(cfg)) {
        return EXIT_FAILURE;
    }

    LOG_I("main: initialised photohosting");

    struct sue_event_selector selector;
    sue_event_selector_init(&selector);

    TcpServer tcp_server(cfg, selector);
    if (!tcp_server.Init()) {
        return EXIT_FAILURE;
    }

    HttpServer http_server(cfg, selector, &photohosting);
    if (!http_server.Init()) {
        return EXIT_FAILURE;
    }

    tcp_server.SetApplicationLevelSessionManager(&http_server);

    LOG_I("main: initialised http");

    if (cfg.runas && !change_user(cfg.runas)) {
        return EXIT_FAILURE;
    }

    try {
        sue_event_selector_go(&selector);
    } catch (const Exception &err) {
        LOG_E("fatal: %s", err.GetErrMsg());
        return EXIT_FAILURE;
    }

    return 0;
}
