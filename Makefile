CC = gcc
FLAGS = -Wall -fpic -g

all: test1 test2 ktjoin tsjfo t128

t128 : test_128_threads.c libuserthread.so
	$(CC) -o t128 test_128_threads.c -L. -luserthread

tsjfo: test_sjf_order.c libuserthread.so
	$(CC) -o tsjfo test_sjf_order.c -L. -luserthread

ktjoin: test_join_chain.c libuserthread.so
	$(CC) -o ktjoin test_join_chain.c -L. -luserthread

test1: test1.c libuserthread.so
	$(CC) -o test1 test1.c -L. -luserthread

test2: test2.c libuserthread.so
	$(CC) -o test2 test2.c -L. -luserthread

libuserthread.so : userthread.o pqueue.o tnode.o
	$(CC) -o libuserthread.so userthread.o pqueue.o tnode.o -shared

userthread.o: userthread.c userthread.h pqueue.c pqueue.h tnode.c tnode.h
	$(CC) $(FLAGS) -c userthread.c pqueue.c tnode.c

clean:
	rm *.o libuserthread.so test1 test2 ktjoin tsjfo t128
