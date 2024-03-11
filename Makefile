CC=gcc
CFLAGS = -O2 -g -std=c11 -pedantic -Wall -Wextra

.PHONY: clean all

all: ian-proj1 example

ian-proj1: ian-proj1.o
	$(CC) $(CFLAGS) $^ -o $@ -lelf

ian-proj1.o: ian-proj1.c
	$(CC) $(CFLAGS) -c $^ -o $@ -lelf

exaple: example.o
	$(CC) $(CFLAGS) $^ -o $@

example.o: example.c
	$(CC) $(CFLAGS) -c $^ -o $@

run: ian-proj1 example
	./ian-proj1 example

run-obj: ian-proj1 example.o
	./ian-proj1 example.o

valgrind-run: ian-proj1 example
	valgrind ./ian-proj1 example

valgrind-run-obj: ian-proj1 example.o
	valgrind ./ian-proj1 example.o

clean:
	rm -f ian-proj1 ian-proj1.o example example.o ian-proj1.tar

zip:
	tar -czvf ian-proj1.tar.gz Makefile ian-proj1.c example.c