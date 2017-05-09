#include "log.h"

#include <syslog.h>

pid_t my_pid;
int log_level;


void get_pid_for_logger()
{
    my_pid = getpid();
}

void set_log_level(int _log_level)
{
    log_level = _log_level;
}
