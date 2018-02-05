#include <stddef.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

#include "sue_aloc.h"
#include "sue_sigs.h"

#include "sue_base.h"

enum {   /* local constants, unused elsewhere */
   fdhandlers_size_step = 16 
};

struct sue_event_selector_timeout_item {
    struct sue_timeout_handler *h;
    struct sue_event_selector_timeout_item *next;
};

struct sue_event_selector_signal_item {
    struct sue_signal_handler *h;
    struct sue_event_selector_signal_item *next;
};

struct sue_event_selector_loophook_item {
    struct sue_loop_hook *h;
    struct sue_event_selector_loophook_item *next;
};

void sue_event_selector_init(struct sue_event_selector *s)
{
    s->timeouts = NULL;
    s->signalhandlers = NULL;
    s->fdhandlers = NULL;
    s->fdhandlerssize = 0;
    s->max_fd = -1;
    s->loophooks = NULL;
    s->breakflag = 0;
}

void sue_event_selector_done(struct sue_event_selector *s)
{
    while(s->timeouts) {
        struct sue_event_selector_timeout_item *tmp = s->timeouts;
        sue_free(s->timeouts);
        s->timeouts = tmp;
    }
    while(s->signalhandlers) {
        struct sue_event_selector_signal_item *tmp = s->signalhandlers;
        sue_free(s->signalhandlers);
        s->signalhandlers = tmp;
    }
    if(s->fdhandlers)
        sue_free(s->fdhandlers);
    while(s->loophooks) {
        struct sue_event_selector_loophook_item *tmp = s->loophooks;
        sue_free(s->loophooks);
        s->loophooks = tmp;
    }
}

void sue_event_selector_register_fd_handler(struct sue_event_selector *s,
                                            struct sue_fd_handler *h)
{
    if(s->fdhandlerssize <= h->fd) { /* needs resize */
        int oldsz = s->fdhandlerssize;
        struct sue_fd_handler **tmp;
        int i;
        s->fdhandlerssize += fdhandlers_size_step;
        tmp = sue_malloc(sizeof(*tmp) * s->fdhandlerssize);
        for(i = 0; i < s->fdhandlerssize; i++)
            tmp[i] = i < oldsz ? s->fdhandlers[i] : NULL;
        if(s->fdhandlers)
            sue_free(s->fdhandlers);
        s->fdhandlers = tmp; 
    }
    s->fdhandlers[h->fd] = h;
    if(h->fd > s->max_fd)
        s->max_fd = h->fd;
}

void sue_event_selector_remove_fd_handler(struct sue_event_selector *s,
                                          struct sue_fd_handler *h)
{
    SUE_ASSERT(h->fd < s->fdhandlerssize && s->fdhandlers[h->fd] == h);
    s->fdhandlers[h->fd] = NULL;
    if(h->fd == s->max_fd) {
        while(s->max_fd >= 0 && s->fdhandlers[s->max_fd])
            s->max_fd--;
    }
}

static int sth_earlier_than(const struct sue_timeout_handler *a,
                            int sec, int usec)
{
    return (a->sec < sec) || (a->sec == sec && a->usec < usec);
}

void sue_event_selector_register_timeout_handler(struct sue_event_selector *s,
                                               struct sue_timeout_handler *h)
{
    struct sue_event_selector_timeout_item **p = &(s->timeouts);
    struct sue_event_selector_timeout_item *tmp;
    while(*p && sth_earlier_than(h, (*p)->h->sec, (*p)->h->usec))
        p = &((*p)->next);
    tmp = sue_malloc(sizeof(*tmp));
    tmp->h = h;
    tmp->next = *p;
    *p = tmp;
}

void sue_event_selector_remove_timeout_handler(struct sue_event_selector *s,
                                               struct sue_timeout_handler *h)
{
    struct sue_event_selector_timeout_item **p = &(s->timeouts);
    while(*p && (*p)->h != h) {
        if(sth_earlier_than((*p)->h, h->sec, h->usec))
            return;
        p = &((*p)->next);
    }
    if(*p) {
        struct sue_event_selector_timeout_item *tmp = *p;
        *p = tmp->next;
        sue_free(tmp);
    }
}

void sue_event_selector_register_signal_handler(struct sue_event_selector *s,
                                                struct sue_signal_handler *h)
{
    struct sue_event_selector_signal_item *tmp;
    tmp = sue_malloc(sizeof(*tmp));
    tmp->h = h;
    tmp->next = s->signalhandlers;
    s->signalhandlers = tmp;
    sue_signals_add(h->signo);
}

void sue_event_selector_remove_signal_handler(struct sue_event_selector *s,
                                              struct sue_signal_handler *h)
{
    struct sue_event_selector_signal_item **p = &(s->signalhandlers);
    while(*p) {
        if((*p)->h == h) {
            struct sue_event_selector_signal_item *tmp = *p;
            *p = tmp->next;
            sue_signals_remove(tmp->h->signo);
            sue_free(tmp);
            return;   /* yes, only one instance is removed */
        }
        p = &((*p)->next);
    }
}

void sue_event_selector_register_loop_hook(struct sue_event_selector *s,
                                           struct sue_loop_hook *h)
{
    struct sue_event_selector_loophook_item *tmp;
    tmp = sue_malloc(sizeof(*tmp));
    tmp->h = h;
    tmp->next = s->loophooks;
    s->loophooks = tmp;
}

void sue_event_selector_remove_loop_hook(struct sue_event_selector *s,
                                         struct sue_loop_hook *h)
{
    struct sue_event_selector_loophook_item **p = &(s->loophooks);
    while(*p) {
        if((*p)->h == h) {
            struct sue_event_selector_loophook_item *tmp = *p;
            *p = tmp->next;
            sue_free(tmp);
            return;   /* yes, only one instance is removed */
        }
        p = &((*p)->next);
    }
}

struct select_sets {
    fd_set rd, wr, ex;
    unsigned has_rd:1, has_wr:1, has_ex:1;
};

static void clear_select_sets(struct select_sets *sets)
{
    FD_ZERO(&(sets->rd));
    FD_ZERO(&(sets->wr));
    FD_ZERO(&(sets->ex));
    sets->has_rd = 0;
    sets->has_wr = 0;
    sets->has_ex = 0;
}

static void fill_select_sets(struct sue_event_selector *s,
                             struct select_sets *sets)
{
    int i;
    clear_select_sets(sets);
    for(i = 0; i <= s->max_fd; i++) {
        if(s->fdhandlers[i]) {
            if(s->fdhandlers[i]->want_read) {
                sets->has_rd = 1;
                FD_SET(i, &(sets->rd));
            }
            if(s->fdhandlers[i]->want_write) {
                sets->has_wr = 1;
                FD_SET(i, &(sets->wr));
            }
            if(s->fdhandlers[i]->want_except) {
                sets->has_ex = 1;
                FD_SET(i, &(sets->ex));
            }
        }
    }
}

static void compute_timeout(const struct sue_timeout_handler *ts,
                            struct timespec *spec)
{
    struct timeval current;
    gettimeofday(&current, NULL /* timezone unused */);
    int a_sec, a_usec;
    if(ts->sec < current.tv_sec ||
        (ts->sec == current.tv_sec && ts->usec <= current.tv_usec))
    {
        spec->tv_sec = 0;
        spec->tv_nsec = 0;
        return;
    }
    a_sec = ts->sec - current.tv_sec;
    a_usec = ts->usec - current.tv_usec;
    if(a_usec < 0) {
        a_usec += 1000000;
        a_sec -= 1;
    }
    spec->tv_sec = a_sec;
    spec->tv_nsec = a_usec * 1000;
}

static void
enter_signal_section(const struct sue_event_selector_signal_item *hs,
                          sigset_t *saved_mask)
{
    sigset_t ps_mask;
    sigemptyset(&ps_mask);
    while(hs) {
        sigaddset(&ps_mask, hs->h->signo);
        hs = hs->next;
    }
    sigprocmask(SIG_BLOCK, &ps_mask, saved_mask);
}

static void leave_signal_section(const sigset_t *saved_mask)
{
    sigprocmask(SIG_SETMASK, saved_mask, NULL);
}

static void
handle_signal_events(const struct sue_event_selector_signal_item *hs)
{
    const struct sue_event_selector_signal_item *p = hs;
    while(p) {
        const struct sue_event_selector_signal_item *nx = p->next;
        int cnt = sue_signals_get_counter(p->h->signo);
        if(cnt > 0)
            p->h->handle_signal(p->h, cnt);
        p = nx;
    }
    sue_signals_zero_counters();
}

void handle_timeouts(struct sue_event_selector_timeout_item **ts)
{
    struct timeval ct;
    gettimeofday(&ct, 0 /* timezone unused */);
    while(*ts && sth_earlier_than((*ts)->h, ct.tv_sec, ct.tv_usec)) {
        struct sue_event_selector_timeout_item *tmp = *ts;
        *ts = tmp->next;
        tmp->h->handle_timeout(tmp->h);
        sue_free(tmp);
    }
    /* we only remove items from the start of the list; even if the handler
       changes the list, nothing wrong will happen, as we don't store
       any information here, we always look at the main object's pointer */
}

static void
handle_fds(struct sue_event_selector *s, struct select_sets *sets)
{
    int i;
    for(i = 0; i <= s->max_fd; i++) {
        struct sue_fd_handler *curr_handler = s->fdhandlers[i];
        if(curr_handler) {
            int rd = FD_ISSET(i, &sets->rd);
            int wr = FD_ISSET(i, &sets->wr);
            int ex = FD_ISSET(i, &sets->ex);
            if(rd || wr || ex)
                curr_handler->handle_fd_event(curr_handler, rd, wr, ex);
        }
    }
}

static void handle_loophooks(struct sue_event_selector_loophook_item *lhs)
{
    struct sue_event_selector_loophook_item *p;
    p = lhs;
    while(p) {
        struct sue_event_selector_loophook_item *nx = p->next;
        p->h->loop_hook(p->h);
        p = nx;
    }
    /* such a construct with 'nx' pointer is needed for the case the hook
       decides to remove itself from the list; should it decide to add
       something, there's nothing wrong with it, as addition is always
       done into the beginning of the list
     */
}


int sue_event_selector_go(struct sue_event_selector *s)
{
    while(!s->breakflag) {
        struct select_sets sets;
        struct timespec tspec;
        sigset_t ps_mask;
        int res;


            /* Within the following section, all signals handled with the
               event selector are blocked (masked), except for the duration
               of pselect call, when they're unblocked.  Please note that 
               we actually check/handle signals twice, which allows to 
               leave the signals unmasked outside of the section.
               Actually, the first call to handle_signal_events processes
               the signals received during the event handling, and the
               second call handles signals happened when we were blocked
               in pselect.
             */
        if(s->signalhandlers) {
            enter_signal_section(s->signalhandlers, &ps_mask);
            handle_signal_events(s->signalhandlers);
        }
        fill_select_sets(s, &sets);
        if(s->timeouts)
            compute_timeout(s->timeouts->h, &tspec);
        res = pselect(s->max_fd+1,
                      sets.has_rd ? &sets.rd : NULL,
                      sets.has_wr ? &sets.wr : NULL,
                      sets.has_ex ? &sets.ex : NULL,
                      s->timeouts ? &tspec : NULL,
                      s->signalhandlers ? &ps_mask : NULL);
        if(res == -1 && errno != EINTR)
            return -1;
        if(s->signalhandlers) {
            if(res == -1 && errno == EINTR)
                handle_signal_events(s->signalhandlers);
            leave_signal_section(&ps_mask);
        }

        handle_timeouts(&(s->timeouts));
        if(res > 0)
            handle_fds(s, &sets);
        handle_loophooks(s->loophooks);
    }
    return 0;
}

void sue_event_selector_break(struct sue_event_selector *s)
{
    s->breakflag = 1;
}

void sue_timeout_handler_set_from_now(struct sue_timeout_handler *th,
                                      long a_sec, long a_usec)
{
    enum { max_usec = 1000000 };
    struct timeval current;
    gettimeofday(&current, 0 /* timezone unused */);
    th->sec = current.tv_sec + a_sec; 
    th->usec = current.tv_usec + a_usec;
    if(th->usec >= max_usec) {
        th->sec += th->usec / max_usec;
        th->usec %= max_usec;
    }
}

