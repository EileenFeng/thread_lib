#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "tnode.h"


thrd* new_thrd(int tid) {
  thrd* td = (thrd*)malloc(sizeof(thrd));
  if(td == NULL) {
    return NULL;
  }
  td->tid = tid;
  td->last_run = NOTSET;
  td->index = 0;
  td->wait_tids = malloc(sizeof(int) * ARRSIZE);
  td->wait_index = NOTSET;
  td->wait_size = ARRSIZE;
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

  if (tn->td->uc.uc_stack.ss_sp != NULL) {
    free(tn->td->uc.uc_stack.ss_sp); // free the stack
  }
  free(tn->td->wait_tids);
  free(tn->td);
  free(tn);
}

thrd* (*new_thread) (int) = &new_thrd;
tnode* (*new_tnode) (thrd*, tnode*) = &new_node;
void (*free_tnode) (tnode*) = &free_node;


/*
void hello() {
  printf("Hello here\n");
}

int main() {
  printf("1");
  
  printf("2");
  ucontext_t uc;
  void* stack = malloc(2048);
  uc.uc_stack.ss_sp = stack;
  uc.uc_stack.ss_size = 2048;
  uc.uc_stack.ss_flags = SS_DISABLE;
  getcontext(&uc);
  makecontext(&uc, hello, 0);
  printf("3");
  thrd* td = new_thrd(0, uc);
  printf("4");
  tnode* tn = new_node(td, NULL);
  printf("before free\n");
  free_node(tn);
}
*/
