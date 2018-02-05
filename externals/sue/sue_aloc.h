#ifndef SUE_ALOC_H_SENTRY
#define SUE_ALOC_H_SENTRY

/*! \file sue_aloc.h
    \brief The SUE library custom memory allocation feature
   
    All SUE functions use sue_malloc and sue_free instead of malloc and
    free.  The sue_malloc function is not intended to ever return NULL,
    so it is not checked anywhere.  Should the stdlib's malloc return
    NULL, the default sue_malloc calls the sue_malloc_error function,
    which prints a message to stderr and exits.  Actually, once your
    application is out of memory, nothing good can be done after that;
    malloc might return NULL when invoked with zero parameter, but SUE
    never calls sue_malloc with zero.

    There are defaults for all these functions; default sue_malloc calls
    the stdlib's malloc and in case of NULL calls sue_malloc_error; the
    default sue_free simply calls stdlib's free; the default
    sue_malloc_error fprintf's to stderr a message and then calls exit(3).

    You can replace these functions however you want.  Please note
    sue_malloc_error SHOULD NOT return; but if it does, then sue_malloc
    returns NULL in which case the library will probably crash, as this
    case is never checked.

    If you decide to replace the functions, perhaps you should either
    replace sue_malloc_error, or replace both sue_malloc and sue_free,
    making sure your version of sue_malloc never returns NULL.
 */

extern void * (*sue_malloc)(int);
extern void (*sue_free)(void *);
extern void (*sue_malloc_error)(void);

#endif

