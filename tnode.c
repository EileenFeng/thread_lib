#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "/usr/include/valgrind/valgrind.h"
#include "tnode.h"
#define STACKSIZE (256*1024)

extern count = 0;

thrd* new_thrd(int tid, ucontext_t* uc) {
  thrd* td = (thrd*)malloc(sizeof(thrd));
  if(td == NULL) {
    return NULL;
  }
  td->uc = uc;
  td->tid = tid;
  td->last_run = NOTSET;
  td->index = 0;
  td->start = malloc(sizeof(struct timeval));
  td->wait_tids = malloc(sizeof(int) * ARRSIZE);
  td->wait_index = NOTSET;
  td->wait_size = ARRSIZE;
  for(int i = 0; i < RECORD_NUM; i++) {
    td->last_thr_run[i] = -1;
  }
  return td;
}

static thrd* copythread(thrd* td) {
  thrd* newthread = malloc(sizeof(thrd));
  if(newthread == NULL) { return NULL; }
  newthread->uc = malloc(sizeof(ucontext_t));
  memcpy(newthread->uc, td->uc, sizeof(ucontext_t));
  void* s = malloc(STACKSIZE);
  newthread->uc->uc_stack.ss_sp = s;
  newthread->uc->uc_stack.ss_size = STACKSIZE;
  newthread->valgrindid = VALGRIND_STACK_REGISTER(s, s+STACKSIZE);    
  memcpy(newthread->uc->uc_stack.ss_sp, td->uc->uc_stack.ss_sp, STACKSIZE);

  //newthread->uc = td->uc;
  newthread->tid = td->tid;
  newthread->state = td->state;
  newthread->priority = td->priority;
  newthread->last_run = td->last_run;
  newthread->index = td->index;
  newthread->wait_size = td->wait_size;
  newthread->wait_index = td->wait_index;

  newthread->wait_tids = malloc(sizeof(int) * td->wait_size);
  newthread->start = malloc(sizeof(struct timeval));
  memcpy(newthread->start, td->start, sizeof(struct timeval));
  memcpy(newthread->wait_tids, td->wait_tids, sizeof(int) * td->wait_size);

   for(int i = 0; i < td->index; i++) {
    newthread->last_thr_run[i] = td->last_thr_run[i];
  }
  return newthread;
}

static tnode* new_node(thrd* td, tnode* next) {
  count++;
  tnode* tn = (tnode*)malloc(sizeof(tnode));
  if(tn == NULL) {
    perror("Error malloc new_node: ");
    return NULL;
  }
  tn->td = td;
  tn->next = next;
  //printf(" CCCCCCCCCCCCCCcc count is %d\n", count);
  return tn;
}

static void free_node(tnode* tn) {
  if(tn == NULL) {
    return;
  }

  if (tn->td->uc->uc_stack.ss_sp != NULL) {
    VALGRIND_STACK_DEREGISTER(tn->td->valgrindid);   
    free(tn->td->uc->uc_stack.ss_sp); // free the stack
    tn->td->uc->uc_stack.ss_sp = NULL;
  }
  if(tn->td->uc != NULL) {
    free(tn->td->uc);
    tn->td->uc = NULL;
  }
  free(tn->td->start);
  free(tn->td->wait_tids);
  free(tn->td);
  free(tn);
}

thrd* (*new_thread) (int, ucontext_t*) = &new_thrd;
tnode* (*new_tnode) (thrd*, tnode*) = &new_node;
void (*free_tnode) (tnode*) = &free_node;
thrd* (*copy_thread) (thrd*) = &copythread;
