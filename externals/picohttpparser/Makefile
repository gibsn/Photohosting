CFLAGS = -g -Wall
STATIC_LIB = libpico.a


static: $(STATIC_LIB)

$(STATIC_LIB): picohttpparser.o
	$(AR) r $@ $^

picohttpparser.o: picohttpparser.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f libpico.a
	rm -f picohttpparser.o

.PHONY: static picohttpparser clean
