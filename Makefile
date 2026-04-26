CC=gcc
CFLAGS=-Wall -Wextra -Werror -g

all:
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/btree.o -c src/btree.c
	$(CC) $(CFLAGS) -o bin/avltree.o -c src/avltree.c
	$(CC) $(CFLAGS) -o bin/main bin/btree.o bin/avltree.o src/main.c

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite,possible --error-exitcode=1 ./bin/main
