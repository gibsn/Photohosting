#include "log.h"

#include <string.h>

#include "cfg.h"


pid_t my_pid;
int log_level;
bool colour;


static void set_log_level(const char *_log_level)
{
    if (!_log_level) {
        log_level = 0;
        return;
    }

    if (!strcmp(_log_level, "LOG_ERR")) {
        log_level = LOG_ERR;
    } else if (!strcmp(_log_level, "LOG_WARNING")) {
        log_level = LOG_WARNING;
    } else if (!strcmp(_log_level, "LOG_NOTICE")) {
        log_level = LOG_NOTICE;
    } else if (!strcmp(_log_level, "LOG_INFO")) {
        log_level = LOG_INFO;
    } else if (!strcmp(_log_level, "LOG_DEBUG")) {
        log_level = LOG_DEBUG;
    }
}


void init_logger(const Config &cfg)
{
    my_pid = getpid();
    colour = cfg.log_colour;

    set_log_level(cfg.log_level);
}
