#include "log.h"

#include <syslog.h>

pid_t my_pid;
int max_log_level;


void get_pid_for_logger()
{
    my_pid = getpid();
}

void set_max_log_level(int _log_level)
{
    max_log_level = _log_level;
}
