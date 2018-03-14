#ifndef PQUEUE_H_
#define PQUEUE_H_
#include "tnode.h"

typedef struct pqueue_record* pqueue;

pqueue (*new_queue) ();

tnode* (*get_head) (pqueue);

int (*get_size) (pqueue);

void (*insert) (pqueue, tnode*);

void (*insert_tail) (pqueue, tnode*); 

void (*insert_after) (pqueue, tnode*, tnode*);

void (*pop_head) (pqueue); // does not free head

void (*delete_head) (pqueue); // free head

void (*remove_node) (pqueue, int);

void (*free_queue) (pqueue);

#endif
