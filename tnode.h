#ifndef TNODE_H_
#define TNODE_H_
#include <ucontext.h>

#define RECORD_NUM 3

typedef struct thrd {
  int tid;
  int state; // scheduled or stopped
  int schedule; // scheduling algorithm
  int priority;
  double last_run; // the time interval of the last run
  double last_thr_run[RECORD_NUM]; // the last three run times
  ucontext_t uc;

  int wait_tid; // the tid of the thread that this thread joined
} thrd;

typedef struct tnode {
  thrd* td; // needs to malloc
  struct tnode* next;
} tnode;

//static thrd* new_thrd(int tid, ucontext_t);

//static tnode* new_node(thrd*, tnode*);

//static void free_node(tnode*);

thrd* (*new_thread) (int, ucontext_t);

tnode* (*new_tnode) (thrd*, tnode*);

void (*free_tnode) (tnode*);

#endif
