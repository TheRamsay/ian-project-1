CC=gcc
CFLAGS = -O2 -g -std=c11 -pedantic -Wall -Wextra

.PHONY: clean

all: ian-proj1 test

ian-proj1: ian-proj1.o
	$(CC) $(CFLAGS) $^ -o $@ -lelf

ian-proj1.o: ian-proj1.c
	$(CC) $(CFLAGS) -c ian-proj1.c -o ian-proj1.o -lelf

test: test.c
	$(CC) $(CFLAGS) -o test test.c

run: ian-proj1 test
	./ian-proj1 test

clean:
	rm -f ian-proj1 ian-proj1.o test