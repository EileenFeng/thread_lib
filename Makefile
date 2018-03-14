CC = gcc
FLAGS = -Wall -fpic -g

test: test1.c libuserthread.so
	$(CC) -o test test1.c -L. -luserthread

libuserthread.so : userthread.o pqueue.o tnode.o
	$(CC) -o libuserthread.so userthread.o pqueue.o tnode.o -shared

userthread.o: userthread.c userthread.h pqueue.c pqueue.h tnode.c tnode.h
	$(CC) $(FLAGS) -c userthread.c pqueue.c tnode.c

clean:
	rm *.o libuserthread.so
