CFLAGS = -g -Wall -fPIC -O3
STATIC_LIB = libmultipart.a


static: $(STATIC_LIB)

$(STATIC_LIB): multipart_parser.o
	$(AR) r $@ $^

multipart_parser.o: multipart_parser.c multipart_parser.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(STATIC_LIB)
	rm -f multipart_parser.o

.PHONY: static clean
