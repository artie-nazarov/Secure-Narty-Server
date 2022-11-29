# CSE130 Assignment 1 - Makefile
CC = $(COMPILER) $(FLAGS)
COMPILER = clang
FLAGS = -Wall -Werror -Wextra -pedantic

all: httpserver

httpserver: httpserver.o bind.o http_object.o
	$(CC) -o $@ $^

httpserver.o: httpserver.c bind.c http_object.c
	$(CC) -c $^

bind.o: bind.c bind.h
	$(CC) -o $<

http_object.o: http_object.c http_object.h
	$(CC) -o $<

format:
	clang-format -i -style=file *.c *.h

clean:
	rm -f *.o httpserver 

