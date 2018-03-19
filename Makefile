CC = gcc
FLAGS = -Wall -fpic -g

all: fifoyield fyldself fifocreate sjforder tpcreate tpschedule basicfree tpjoin tpalarm tpyinside

basicfree: test_basic_free.c libuserthread.so
	$(CC) -o basicfree test_basic_free.c -L. -luserthread

tpyinside: test_priority_yield_inside.c libuserthread.so
	$(CC) -o tpyinside test_priority_yield_inside.c -L. -luserthread

tpjoin: test_priority_join_inside.c libuserthread.so
	$(CC) -o tpjoin test_priority_join_inside.c -L. -luserthread

tpalarm: test_priority_alarm.c libuserthread.so
	$(CC) -o tpalarm test_priority_alarm.c -L. -luserthread

tpschedule: test_schedule_priority.c libuserthread.so
	$(CC) -o tpschedule test_schedule_priority.c -L. -luserthread

tpcreate: tpriority_create_join.c libuserthread.so
	$(CC) -o tpcreate tpriority_create_join.c -L. -luserthread

sjforder: test_sjf_order.c libuserthread.so
	$(CC) -o sjforder test_sjf_order.c -L. -luserthread

fifocreate: test_fifo_create.c libuserthread.so
	$(CC) -o fifocreate test_fifo_create.c -L. -luserthread

fifoyield: test_fifo_yield.c libuserthread.so
	$(CC) -o fifoyield test_fifo_yield.c -L. -luserthread

fyldself: test_fifo_yieldself.c libuserthread.so
	$(CC) -o fyldself test_fifo_yieldself.c -L. -luserthread

libuserthread.so : userthread.o pqueue.o tnode.o
	$(CC) -o libuserthread.so userthread.o pqueue.o tnode.o -shared

userthread.o: userthread.c userthread.h pqueue.c pqueue.h tnode.c tnode.h
	$(CC) $(FLAGS) -c userthread.c pqueue.c tnode.c

clean:
	rm *.o libuserthread.so fifoyield fyldself fifocreate sjforder tpcreate tpschedule basicfree tpjoin tpalarm tpyinside
