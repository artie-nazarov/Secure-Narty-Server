# CSE130 Fall 2022 - Natalie Valett, nvalett
# Assignment 0 Makefile

CC = $(COMPILER) $(FLAGS)
COMPILER = clang
FLAGS = -Wall -Werror -Wextra -pedantic

all: queue.o

queue.o: queue.c
	$(CC) -c $<
	
clean:
	rm -f split *.o