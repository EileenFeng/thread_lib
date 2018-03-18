#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "pqueue.h"

enum state {
  CREATED,
  SCHEDULED,
  STOPPED,
  FINISHED
};


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
    printf("r1 %d\n", pq->size);
    return;
  }
  if(tn == NULL) {
    printf("r2\n");
    return;
  }
  tnode* thead = pq->head;
  if(tn->td->priority < thead->td->priority && thead->td->state != FINISHED) {
    tn->next = thead;
    pq->head = tn;
    pq->size ++;
    printf("r3\n");
    return;
  }
  while(thead->next != NULL) {
    if(thead->next->td->priority <= tn->td->priority) {
      thead = thead->next;
    } else if(thead->next->td->state == FINISHED) {
      thead = thead->next;
    } else {
      break;
    }
  }
  printf("last tid %d\n", thead->td->tid);
  tnode* temp = thead->next;
  printf("temp is NULL? %d\n", temp == NULL);
  thead->next = tn;
  tn->next = temp;
  pq->size++;
  printf("r4 %d\n", pq->size);
  return;
}

static void arrange(pqueue pq, tnode* target) {
  tnode* curhead = get_head(pq);
  if(curhead->td->tid == target->td->tid) {
    if(curhead->td->priority < curhead->next->td->priority) {
      return;
    }
  }
  tnode* pre = NULL;
  tnode* cur = NULL;
  tnode* nex = NULL;
  cur = get_head(pq);
  while(cur != NULL && cur->td->tid != target->td->tid) {
    pre = cur;
    cur = nex;
    if(cur != NULL) {
      nex = cur->next;
    } else {
      nex = NULL;
    }
  }
  if(cur == NULL) {
    return;
  } else {
    nex = cur->next;
    if(pre != NULL) {
      pre->next = nex;
    } else {
      pq->head = nex;
    }
    pq->size --;
    insert_node(pq, target);
  }
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
  printf("queue size %d\n", pq->size);
  if(pq->size != 0) {
    printf("inside of deletehead deleting %d with priority %f\n", pq->head->td->tid, pq->head->td->priority);
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

/*
extern pqueue (*new_queue)() = &new_q;
extern tnode* (*get_head) (pqueue) = &getHead;
extern int (*get_size) (pqueue) = &getsize;
extern void (*insert) (pqueue, tnode*) = &insert_node;
extern void (*insert_tail) (pqueue, tnode*) = &add_tail;
extern void (*remove_node) (pqueue, int) = &remove_n;
extern void (*free_queue) (pqueue) = &free_list;
extern void (*insert_after) (pqueue, tnode*, tnode*) = &insertafter;
extern void (*pop_head) (pqueue) = &pophead;
extern void (*delete_head) (pqueue) = &deletehead;
extern tnode* (*find_tid) (pqueue, int) = &findtid;
extern void (*move_head) (pqueue, pqueue) = &movehead_finished;
extern void (*arrange_queue) (pqueue, tnode*) = &arrange;
*/
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
void (*arrange_queue) (pqueue, tnode*) = &arrange; 

