#include <stdlib.h>
#include <stdio.h>
#include "sue_aloc.h"

static void *sue_default_malloc(int size)
{
    void *r = malloc(size);
    if(!r) {
        (*sue_malloc_error)();
        return NULL;
    }
    return r;
}

static void sue_default_free(void *p)
{
    free(p);
}

static void sue_default_malloc_error(void)
{
    fprintf(stderr, "sue: memory allocation failed; panic\n");
    exit(1);
}

void * (*sue_malloc)(int) = &sue_default_malloc;

void (*sue_free)(void *) = &sue_default_free;

void (*sue_malloc_error)(void) = &sue_default_malloc_error;

