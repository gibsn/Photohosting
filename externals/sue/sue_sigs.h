#ifndef SUE_SIGS_H_SENTRY
#define SUE_SIGS_H_SENTRY

/*! \file sue_sigs.h
    \brief The SUE library signal handling
 */

void sue_signals_init();
void sue_signals_add(int signo);
void sue_signals_remove(int signo);
int sue_signals_get_counter(int signo);
void sue_signals_zero_counters();

#endif

