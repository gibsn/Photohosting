#include "log.h"

#include <string.h>

pid_t my_pid;
int log_level;


void get_pid_for_logger()
{
    my_pid = getpid();
}

void set_log_level(const char *_log_level)
{
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
