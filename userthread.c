#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "userthread.h"
#include "pqueue.h"

#define H -1
#define M 0
#define L 1

/****************************** globals ***********************************/
// enums
enum state {CREATED, SCHEDULED, STOPPED, FINISHED};

// priority queue for FIFO and sjf
static pqueue fqueue, squeue;

//priority queues for priority scheduling
static pqueue first, second, third;


static int plcy = -1; // policy of current scheduling 
static int tidcount = 0;
static tnode* head = NULL;

/************************ function prototypes *****************************/

int thread_libinit(int policy) {
  plcy = policy;
  if(policy == FIFO) {
    fqueue = new_queue();
    if(fqueue == NULL) {
      return -1;
    }
  } else if (policy == SJF) {
    squeue = new_queue();
    if(squeue == NULL) {
      return -1;
    }
  } else if (policy == PRIORITY) {
    first = new_queue();
    second = new_queue();
    thrid = new_queue();
    if(first == NULL || second == NULL || third == NULL) {
      return -1;
    }
  }  
}


int thread_libterminated() {
  free_queue(fqueue);
  free_tnode(head);

}

int main(int argc, char** argv) {
  ucontext_t uc;
  void* stack = malloc(2048);
  uc.uc_stack.ss_sp = stack;
  uc.uc_stack.ss_size = 2048;
  getcontext(&uc);
  thrd* td = new_thread(0, uc);
  tnode* tn = new_tnode(td, NULL);
  pqueue pq = new_queue();
  insert(pq, tn);
  tnode* h = get_head(pq);
  printf("head tid is %d\n", h->td->tid);
}
