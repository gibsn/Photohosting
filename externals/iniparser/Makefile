CFLAGS  += -fPIC -Wall -Wextra -ansi -pedantic -g

# Ar settings to build the library
AR	    ?= ar
ARFLAGS = rcv

SHLD = ${CC} ${CFLAGS}
LDSHFLAGS = -shared -Wl,-Bsymbolic
LDFLAGS += -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib


SRCS = src/iniparser.c \
	   src/dictionary.c

OBJS = $(SRCS:.c=.o)

STATIC_LIB = libiniparser.a


static: $(STATIC_LIB)

libiniparser.a:	$(OBJS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -rf $(OBJS) $(STATIC_LIB)
