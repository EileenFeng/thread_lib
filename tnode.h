#ifndef TNODE_H_
#define TNODE_H_
#include <ucontext.h>

#define RECORD_NUM 3
#define ARRSIZE 1024
#define NOTSET -2

typedef struct thrd {
  int tid;
  int state; // scheduled or stopped
  int schedule; // scheduling algorithm
  int priority;
  int index; // writing index to the last_thr_run array
  double last_run; // the time interval of the last run
  double last_thr_run[RECORD_NUM]; // the last three run times
  ucontext_t* uc;

  // needs to change to all threads waiting for me
  int* wait_tids;
  int wait_size;
  int wait_index;
} thrd;

typedef struct tnode {
  thrd* td; // needs to malloc
  struct tnode* next;
} tnode;

thrd* (*new_thread) (int, ucontext_t*);

tnode* (*new_tnode) (thrd*, tnode*);

void (*free_tnode) (tnode*);

#endif
