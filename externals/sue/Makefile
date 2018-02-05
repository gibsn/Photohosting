CC = gcc
CXX = g++
CFLAGS = -g -Wall -O0
CXXFLAGS = $(CFLAGS)

CSOURCES = sue_base.c sue_aloc.c sue_sigs.c
CXXSOURCES = 
HEADERS = $(CSOURCES:.c=.h) $(CXXSOURCES:.cpp=.hpp)
OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)


HEADERFILES = $(HEADERS)	


default: libsue.a
	:

static: libsue.a


%.o:	%.cpp 
	$(CXX) $(CXXFLAGS) -c $<

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libsue.a:	$(OBJECTS)
	$(AR) crs $@ $^ 

clean:
	rm -f *.o *.bin *.a *~ buf deps.mk

deps.mk: $(CSOURCES) $(CXXSOURCES) Makefile
	$(CC) -MM $(CSOURCES) > $@
	[ x != x"$(CXXSOURCES)" ] && $(CXX) -MM $(CXXSOURCES) >> $@ || true

ifneq (clean, $(MAKECMDGOALS))
ifneq (tags, $(MAKECMDGOALS))
-include deps.mk
endif
endif
