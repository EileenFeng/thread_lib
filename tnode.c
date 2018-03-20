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

static tnode* new_node(thrd* td, tnode* next) {
  tnode* tn = (tnode*)malloc(sizeof(tnode));
  if(tn == NULL) {
    perror("Error malloc new_node: ");
    return NULL;
  }
  tn->td = td;
  tn->next = next;
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
