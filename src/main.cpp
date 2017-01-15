#include <stdio.h>

#include "common.h"
#include "http_server.h"
#include "log.h"

/*TODO:
  move select to another module
  chdir to path from cfg
  add cfg
  fix diag msg when fork exits
  linux doesnt have strnstr, wont compile
*/

int main(int argc, char **argv)
{
    Config cfg;

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;
    cfg.Check();
    LOG_I("Initialised config");

    //global variables from log.h
    get_pid_for_logger();
    set_max_log_level(cfg.max_log_level);
    LOG_I("Initialised logging");

    LOG_I("Starting server");
    HttpServer server(cfg);
    server.Init();
    server.ListenAndServe();

    return 0;
}
