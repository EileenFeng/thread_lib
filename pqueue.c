#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "pqueue.h"

typedef struct pqueue_record{
  tnode* head;
  int size;
} pqueue_record;

static pqueue new_q() {
  pqueue pq = (pqueue_record*)malloc(sizeof(pqueue_record));
  if(pq == NULL) {
    return NULL;
  }
  pq->head = NULL;
  pq->size = 0;
  return pq;
}

static int getsize(pqueue pq) {
  return pq->size;
}

static void insert_node(pqueue pq, tnode* tn) {
  if(pq->size == 0) {
    pq->head = tn;
    pq->size = 1;
    return;
  }
  tnode* thead = pq->head;
  if(tn->td->priority < thead->td->priority) {
    tn->next = thead;
    pq->head = tn;
    pq->size ++;
    return;
  }
  while(thead->next != NULL && thead->next->td->priority < tn->td->priority) {
    thead = thead->next;
  }
  tn->next = thead->next;
  thead->next = tn->next;
  pq->size++;
  return;
}

static void add_tail(pqueue pq, tnode* tn) {
  if(pq->size == 0) {
    pq->head = tn;
    pq->size = 1;
    return;
  }
  tnode* thead = pq->head;
  while(thead->next != NULL) {
    thead = thead->next;
  }
  thead->next = tn;
  tn->next = NULL;
  pq->size ++;
}

static void remove_n(pqueue pq, int tid) {
  if(pq->size != 0) {
    tnode* tn = pq->head;
    if(tn->td->tid == tid) {
      tnode* nhead = tn->next;
      pq->head = nhead;
      free_tnode(tn);
      pq->size --;
      return;
    }
    while(tn->next != NULL && tn->next->td->tid != tid) {
      tn = tn->next;
    }
    if(tn->next != NULL) {
      tnode* tnnext = tn->next->next;
      tnode* target = tn->next;
      tn->next = tnnext;
      free_tnode(target);
      pq->size --;
      return;
    }
  }
  return;
}

static void free_list(pqueue pq) {
  if(pq->size != 0) {
    tnode* cur = pq->head;
    tnode* nex = cur->next;
    while(nex != NULL) {
      free_tnode(cur);
      cur = nex;
      nex = nex->next;
    }
    free_tnode(cur);
    free(pq);
    return;
  } else {
    free(pq);
    return;
  }
}

static tnode* getHead(pqueue pq) {
  if(pq->size == 0) {
    return NULL;
  }
  return pq->head;
}

static void insertafter (pqueue pq, tnode* target, tnode* after) {
  tnode* curnext = target->next;
  target->next = after;
  after->next = curnext;
  pq->size ++;
}

static void pophead(pqueue pq) {
  if(pq->size != 0) {
    tnode* curnext = pq->head->next;
    pq->head = curnext;
    pq->size --;
  }
}

static void deletehead(pqueue pq) {
  if(pq->size != 0) {
    printf("in here\n");
    tnode* oldhead = pq->head;    
    tnode* curnext = pq->head->next;
    free_tnode(oldhead);
    pq->head = curnext;
    pq->size --;
    printf("inside deletehead %d\n", pq->size);
  }
}

static void movehead_finished(pqueue pq, pqueue fin) {
  tnode* head = pq->head;
  tnode* newhead = head->next;
  pq->size --;
  pq->head = newhead;
  insert_tail(fin, head);
}

static tnode* findtid(pqueue pq, int tid) {
  if(pq->size == 0) {
    return NULL;
  }
  tnode* thead = pq->head;
  while(thead != NULL) {
    if(thead->td->tid == tid) {
      return thead;
    }
    thead = thead->next;
  }
  return thead;
}


pqueue (*new_queue)() = &new_q;
tnode* (*get_head) (pqueue) = &getHead;
int (*get_size) (pqueue) = &getsize;
void (*insert) (pqueue, tnode*) = &insert_node;
void (*insert_tail) (pqueue, tnode*) = &add_tail;
void (*remove_node) (pqueue, int) = &remove_n;
void (*free_queue) (pqueue) = &free_list;
void (*insert_after) (pqueue, tnode*, tnode*) = &insertafter;
void (*pop_head) (pqueue) = &pophead;
void (*delete_head) (pqueue) = &deletehead;
tnode* (*find_tid) (pqueue, int) = &findtid;
void (*move_head) (pqueue, pqueue) = &movehead_finished;

/*
void hello() {
  printf("haha1 \n");
}


void hello2() {
  printf("haha2 \n");
}


int main() {
  
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
  td->priority = -1;
  td->tid = 0;
  printf("4");
  tnode* tn = new_node(td, NULL);
 
   
  ucontext_t uc1;
  void* stack1 = malloc(2048);
  uc1.uc_stack.ss_sp = stack1;
  uc1.uc_stack.ss_size = 2048;
  uc1.uc_stack.ss_flags = SS_DISABLE;
  getcontext(&uc1);
  makecontext(&uc1, hello2, 0);
  printf("32");
  thrd* td1 = new_thrd(0, uc1);
  td1->tid = 1;
  td1->priority = 2;
  printf("42");
  tnode* tn1 = new_node(td1, NULL);

  
  pqueue newq = new_queue();
  insert(newq, tn1);
  insert(newq, tn);
  printf("head priority is: %d\n", newq->head->td->priority);
  printf("next priority is: %d\n", newq->head->next->td->priority);
  remove_node(newq, 3);
  remove_node(newq, 1);
  printf("hahha head priority is: %d\n", newq->head->td->priority);
  //printf("next priority is: %d\n", newq->head->next->td->priority);
  free_list(newq);
  printf("before free\n");
}
*/
