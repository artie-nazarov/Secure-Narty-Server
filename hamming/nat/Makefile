CC = clang
CFLAGS = -Wall -Wextra  -Werror  -Wpedantic

all: banhammer

banhammer: banhammer.o node.o parser.o speck.o bf.o bv.o ht.o ll.o
	$(CC) -o banhammer banhammer.o node.o parser.o speck.o bf.o bv.o ht.o ll.o -lm

banhammer.o: banhammer.c
	$(CC) $(CFLAGS) -c banhammer.c

node.o: node.c
	$(CC) $(CFLAGS) -c node.c

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

speck.o: speck.c
	$(CC) $(CFLAGS) -c speck.c

bf.o: bf.c
	$(CC) $(CFLAGS) -c bf.c

bv.o: bv.c
	$(CC) $(CFLAGS) -c bv.c

ht.o: ht.c
	$(CC) $(CFLAGS) -c ht.c

ll.o: ll.c
	$(CC) $(CFLAGS) -c ll.c

clean:
	rm -f *.o tsp

format:
	clang-format -i -style=file *.[ch]

clang-format:
	clang-format -i -style=file *.h *.c

scan -build: clean
	scan -build  make
