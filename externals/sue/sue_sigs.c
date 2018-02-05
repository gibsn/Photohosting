#include <signal.h>
#include <stddef.h>  /* for NULL */
#include "sue_sigs.h"

#ifndef _NSIG
#define _NSIG 64   /* usually this constant is defined in system headers */
#endif


static int sighdl_count[_NSIG];  /* currenly active handlers per signal */
static struct sigaction saved_sigactions[_NSIG];
static volatile sig_atomic_t signal_counters[_NSIG];

/* we assume signals are numbered from 1 to _NSIG */

static void the_handler(int signo)
{
    if(signo < 1 || signo > _NSIG)   /* paranoid check :-) */
        return;
    signal_counters[signo-1]++;
}

void sue_signals_init()
{
}

void sue_signals_add(int signo)
{
    if(signo < 1 || signo > _NSIG)
        return;
    if(0 >= sighdl_count[signo-1]++) {
        /* set up the handler! */
        struct sigaction act;
        act.sa_handler = &the_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(signo, &act, saved_sigactions + (signo-1));
    }
}

void sue_signals_remove(int signo)
{
    if(signo < 1 || signo > _NSIG)
        return;
    if(--sighdl_count[signo-1] == 0) {
        /* remove the handler, restore the saved disposition */
        sigaction(signo, saved_sigactions + (signo-1), NULL);
    }
}

int sue_signals_get_counter(int signo)
{
    return signal_counters[signo-1];
}

void sue_signals_zero_counters()
{
    int i;
    for(i = 0; i < _NSIG; i++)
        signal_counters[i] = 0;
}

