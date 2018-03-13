#ifndef PQUEUE_H_
#define PQUEUE_H_
#include "tnode.h"

typedef struct pqueue_record* pqueue;

//static pqueue new_q();

//static void insert_node(pqueue, tnode*); // insert based on priority, the lower the higher priority

//static void add_tail(pqueue, tnode*);

//static void remove_n(pqueue, int); //remove the node with the corresponding tid

//static void free_list(pqueue);

pqueue (*new_queue) ();

tnode* (*get_head) (pqueue);

int (*get_size) (pqueue);

void (*insert) (pqueue, tnode*);

void (*insert_tail) (pqueue, tnode*); 

void (*remove_node) (pqueue, int);

void (*free_queue) (pqueue);

#endif
