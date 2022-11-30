# CSE130 Fall 2022 - Natalie Valett, nvalett
# Assignment 0 Makefile

CC = $(COMPILER) $(FLAGS)
COMPILER = clang
FLAGS = -Wall -Werror -Wextra -pedantic -pthread

all: httpserver

httpserver: httpserver.o bind.o operators.o httperrors.o httprequest.o queue.o
	$(CC) -o httpserver httpserver.o bind.o operators.o httperrors.o httprequest.o queue.o

httpserver.o: httpserver.c
	$(CC) -c $<

bind.o: bind.c
	$(CC) -c $<

operators.o: operators.c
	$(CC) -c $<

httperrors.o: httperrors.c
	$(CC) -c $<

httprequest.o: httprequest.c
	$(CC) -c $<

queue.o: queue.c
	$(CC) -c $<
	
clean:
	rm -f split *.o