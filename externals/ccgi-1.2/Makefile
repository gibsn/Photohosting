CFLAGS = -I .

static: libccgi.a

libccgi.a: ccgi.o
	ar r libccgi.a ccgi.o

ccgi.o: ccgi.c ccgi.h

clean:
	rm -f *.o *.a
