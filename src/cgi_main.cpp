#include "photohosting.h"

#include <unistd.h>
#include <stdlib.h>

#include "auth.h"
#include "cfg.h"
#include "cgi.h"
#include "log.h"

#define PATH_TO_CFG "./photohosting.ini"


int main()
{
    // disabling logs
    set_log_level(0);
    close(2);

    Config cfg;

    if (!cfg.Init(PATH_TO_CFG)) return EXIT_FAILURE;
    cfg.Check();

    Auth auth;
    if (!auth.Init(cfg.path_to_pwd, cfg.path_to_tokens)) return EXIT_FAILURE;

    Photohosting photohosting(cfg, &auth);
    if (!photohosting.Init(cfg)) return EXIT_FAILURE;

    Cgi cgi(cfg, &photohosting);
    if (!cgi.Init()) return EXIT_FAILURE;

    cgi.Routine();

    return 0;
}
