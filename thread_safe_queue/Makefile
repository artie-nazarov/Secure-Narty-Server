# CSE130 Assignment 2 - Makefile
CC = $(COMPILER) $(FLAGS)
COMPILER = clang
FLAGS = -Wall -Werror -Wextra -pedantic

all: queue.o

queue.o: queue.c queue.h
	$(CC) -c $<

format:
	clang-format -i -style=file *.c *.h

clean:
	rm -f *.o 

