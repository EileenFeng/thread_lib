CC = gcc
FLAGS = -Wall -g 

all: threadlib

threadlib : userthread.o pqueue.o tnode.o
	$(CC) -o threadlib userthread.o pqueue.o tnode.o

threadlib.o: userthread.c userthread.h  
	$(CC) -c $(FLAGS) userthread.c 

pqueue.o: pqueue.c pqueue.h
	$(CC) -c $(FLAGS) pqueue.c

tnode.o: tnode.c tnode.h
	$(CC) -c $(FLAGS) tnode.c 

clean:
	rm *.o threadlib
