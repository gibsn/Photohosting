#include "photohosting.h"

#include <unistd.h>

#include "auth.h"
#include "cfg.h"
#include "cgi.h"
#include "log.h"

#define PATH_TO_CFG "./config.ini"


int main()
{
    // disabling logs
    set_log_level(0);
    close(2);

    Config cfg;

    if (!cfg.Init(PATH_TO_CFG)) return -1;
    cfg.Check();

    Auth auth;
    if (!auth.Init(cfg.path_to_pwd, cfg.path_to_tokens)) return -1;

    Photohosting photohosting(cfg, &auth);

    Cgi cgi(cfg, &photohosting);
    if (!cgi.Init()) return -1;

    cgi.Routine();

    return 0;
}
