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

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;

    if (!cfg.Init(cfg.path_to_cfg)) return -1;
    cfg.Check();

    get_pid_for_logger();
    set_log_level(cfg.log_level);
    LOG_I("Initialised logging");
    LOG_I("Initialised config");

    Auth auth;
    auth.Init(cfg.path_to_pwd, cfg.path_to_tokens);
    LOG_I("Initialised auth");

    Photohosting photohosting(cfg.path_to_store, cfg.path_to_css, &auth);

    LOG_I("Starting server");
    HttpServer server(cfg, &photohosting);
    server.Init();

    if (cfg.runas) {
        if (!change_user(cfg.runas)) exit(-1);
    }

    server.ListenAndServe();

    return 0;
}
