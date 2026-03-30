CC=gcc
CFLAGS=-Iinclude -Wall

all:
	$(CC) src/log.c test/test_log.c -o test_log $(CFLAGS) -lpthread

clean:
	rm -f test_log test.log