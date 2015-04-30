CC=clang
CFLAGS=-Wall -Werror -Wextra -pedantic -pedantic-errors -std=c11 -O2 -g
LN_CFLAGS=-O2
MY_CFLAGS=-I/usr/local/include
LIBS=$(LDFLAGS)

all: test lib speed_test

test: uparse.o test.o
	$(CC) uparse.o test.o -o test

test.o: uparse.o test.c
	$(CC) -fPIC $(CFLAGS) -c test.c

speed_test: uparse.o speed_test.o
	$(CC) uparse.o speed_test.o -o speed_test

speed_test.o: uparse.o speed_test.c
	$(CC) -fPIC $(CFLAGS) -c speed_test.c

uparse.o: uparse.c uparse.h
	$(CC) -fPIC $(CFLAGS) -c uparse.c

lib: uparse.o
	$(CC) uparse.o -shared -o libuparse.so
	ar rcs libuparse.a uparse.o

clean:
	rm -f *.o *.so *.a test speed_test
