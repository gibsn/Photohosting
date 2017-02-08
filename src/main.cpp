#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "http_server.h"
#include "log.h"

/*TODO:
  move select to another module
  add cfg
  implement 304 not modified
  runas
*/

int main(int argc, char **argv)
{
    Config cfg;

    srand(time(NULL));

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;
    cfg.Check();
    LOG_I("Initialised config");

    get_pid_for_logger();
    set_max_log_level(cfg.max_log_level);
    LOG_I("Initialised logging");

    LOG_I("Starting server");
    HttpServer server(cfg);
    server.Init();
    server.ListenAndServe();

    return 0;
}
