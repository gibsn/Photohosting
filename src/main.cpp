#include <stdio.h>
#include <stdlib.h>

#include "auth.h"
#include "common.h"
#include "http_server.h"
#include "log.h"


int main(int argc, char **argv)
{
    Config cfg;

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;
    cfg.Check();
    LOG_I("Initialised config");

    get_pid_for_logger();
    set_max_log_level(cfg.max_log_level);
    LOG_I("Initialised logging");

    Auth auth;
    //TODO: from cfg
    auth.Init("./users", cfg.path_to_tokens);
    LOG_I("Initialised auth");

    LOG_I("Starting server");
    HttpServer server(cfg, &auth);
    server.Init();

    if (cfg.runas) {
        if (!change_user(cfg.runas)) exit(-1);
    }

    server.ListenAndServe();

    return 0;
}
