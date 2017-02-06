#ifndef LOG_H_SENTRY
#define LOG_H_SENTRY

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


extern pid_t my_pid;
extern int max_log_level;

//TODO: fix logging, merges when multiple processes are launched (can use syslog)


void get_pid_for_logger();
void set_max_log_level(int);


#define     LOG_ESCAPE(x)          "\033[01;" #x "m"
#define     LOG_COLOUR_GRAY        LOG_ESCAPE(30)
#define     LOG_COLOUR_RED         LOG_ESCAPE(31)
#define     LOG_COLOUR_GREEN       LOG_ESCAPE(32)
#define     LOG_COLOUR_YELLOW      LOG_ESCAPE(33)
#define     LOG_COLOUR_BLUE        LOG_ESCAPE(34)
#define     LOG_COLOUR_PURPLE      LOG_ESCAPE(35)
#define     LOG_COLOUR_CYAN        LOG_ESCAPE(36)
#define     LOG_COLOUR_WHITE       LOG_ESCAPE(37)
#define     LOG_COLOUR_NORMAL      "\033[00m"


#define LOG_E(str, ...) LOG_ANY(LOG_ERR, "ERROR", str, ##__VA_ARGS__)
#define LOG_W(str, ...) LOG_ANY(LOG_WARNING, "WARNING", str, ##__VA_ARGS__)
#define LOG_I(str, ...) LOG_ANY(LOG_NOTICE, "INFO", str, ##__VA_ARGS__)
#define LOG_D(str, ...) LOG_ANY(LOG_DEBUG, "DEBUG", str, ##__VA_ARGS__)


#define LOG_ANY(level, level_str, str, ...) do {                   \
    if (level > max_log_level) break;                              \
                                                                   \
    if (level == LOG_ERR) {                                        \
        fprintf(stdout, LOG_COLOUR_RED);                           \
    } else if (level == LOG_WARNING) {                             \
        fprintf(stdout, LOG_COLOUR_YELLOW);                        \
    } else if (level == LOG_NOTICE) {                              \
        fprintf(stdout, LOG_COLOUR_GREEN);                         \
    } else if (level == LOG_DEBUG) {                               \
        fprintf(stdout, LOG_COLOUR_WHITE);                         \
    }                                                              \
                                                                   \
    time_t t = time(NULL);                                         \
    struct tm *tm = localtime(&t);                                 \
    if (!tm) break;                                                \
    fprintf(stdout, "%02d.%02d.%04d %02d:%02d:%02d ",              \
        tm->tm_mday,                                               \
        tm->tm_mon + 1,                                            \
        tm->tm_year + 1900,                                        \
        tm->tm_hour,                                               \
        tm->tm_min,                                                \
        tm->tm_sec                                                 \
    );                                                             \
                                                                   \
    fprintf(stdout, "[%d]", my_pid);                               \
    fprintf(stdout, "[" level_str "] " LOG_COLOUR_NORMAL str "\n", \
        ##__VA_ARGS__);                                            \
} while(0);

#endif
