#ifndef PQUEUE_H_
#define PQUEUE_H_
#include "tnode.h"

typedef struct pqueue_record* pqueue;

extern pqueue (*new_queue) ();

extern int (*get_size) (pqueue);

extern tnode* (*get_head) (pqueue);

extern void (*insert) (pqueue, tnode*); // insertion based on priority

extern tnode* (*find_tid) (pqueue, int);

extern void (*insert_tail) (pqueue, tnode*);

extern void (*insert_after) (pqueue, tnode*, tnode*);

extern void (*pop_head) (pqueue); // does not free head

extern void (*delete_head) (pqueue); // free head

extern void (*remove_node) (pqueue, int);

extern void (*free_queue) (pqueue);

extern void (*move_head) (pqueue, pqueue);

extern void (*arrange_queue) (pqueue, tnode*);

extern tnode* (*pop_node) (pqueue, tnode*);

#endif
