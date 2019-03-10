#include <stdio.h>
#include <stdlib.h>

#include "auth.h"
#include "cfg.h"
#include "common.h"
#include "http_server.h"
#include "log.h"
#include "photohosting.h"


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

    HttpServer server(cfg, selector, &photohosting);
    if (!server.Init()) {
        return EXIT_FAILURE;
    }

    LOG_I("main: initialised http");

    if (cfg.runas && !change_user(cfg.runas)) {
        return EXIT_FAILURE;
    }

    sue_event_selector_go(&selector);

    return 0;
}
