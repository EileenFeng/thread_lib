#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "userthread.h"
#include "pqueue.h"

#define SUCCESS 0
#define FAIL -1
#define TRUE 1
#define FALSE 0
#define UNKNOWN -10 //used for unkonwn priorities

#define H -1
#define M 0
#define L 1

/****************************** globals ***********************************/
// enums
enum state {CREATED, SCHEDULED, STOPPED, FINISHED};

// priority queue for FIFO and sjf
static pqueue fqueue, squeue;

//priority queues for priority scheduling
static pqueue first, second, third;

//static timeval begintime; // records beginning time of lib init
static struct timeval begintime;
static int init = FALSE;
static int plcy = -1; // policy of current scheduling 
static int tidcount = 0; //counting the number of threads
static int logfd; // the file descriptor of the log file
//static tnode* curhead = NULL;
static pqueue curq = NULL; // keep track of the current queue (for fifo and sjf)
/************************ function prototypes *****************************/
static int scheduler(int policy);
static void stubfunc (void (*func)(void*), void* arg);

static void stubfunc(void (*func)(void*), void* arg) {
  struct timeval begin;
  struct timeval end;
  if(plcy == FIFO) {
    tnode* curhead = get_head(fqueue);
    func(arg);
    curhead->td->state = FINISHED;
    
  } else if (plcy == SJF) {
    tnode* curhead = get_head(squeue);
    gettimeofday(&begin, NULL);
    func(arg);
    gettimeofday(&end, NULL);
    curhead->td->state = FINISHED;
    curhead->td->last_run = end.tv_sec*1000000 + end.tv_usec - begin.tv_sec*1000000 - begin.tv_usec;
    int index = curhead->td->index;
    curhead->td->last_thr_run[index] = curhead->td->last_run;
    curhead->td->index = (index + 1) % RECORD_NUM;
    
  }
  scheduler(plcy);
}

static int scheduler(int policy) {
  struct timeval t;
  if(policy = FIFO) {
    
    if(get_size(fqueue) == 0) {
      printf("fifo queue is emtpy\n");
      return FAIL; 
    }
    
    tnode* oldhead = get_head(fqueue);
    int oheadtid = oldhead->td->tid;
    tnode* newhead = NULL;
    int oldfini = FALSE;
    if(oldhead->td->state != FINISHED) {
      insert_tail(fqueue, oldhead);
      if(oldhead->next != NULL) {
	pop_head(fqueue);
	newhead = get_head(fqueue);	
      }
    } else {
      oldfini = TRUE;
      delete_head(fqueue);
      newhead = get_head(fqueue);
    }

    if(newhead != NULL) {
      gettimeofday(&t, NULL);
      double curtime = t.tv_sec*1000000 + t.tv_usec - begintime.tv_sec*1000000 - begintime.tv_usec;
      if((FILE* stream = fdopen(logfd, "w")) != NULL) { 
	if(oldfini) {
	  fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "FINISHED", oheadtid, UNKNOWN);
	} else {
	  fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "STOPPED", oheadtid, UNKNOWN);
	}
      }
      fclose(stream);
      setcontext(&(newhead->td->uc));
    } else {
      printf("new head is null la\n");
    }
    
  } else {
    printf("not yet implemented\n");
  }

}


int thread_libinit(int policy) {
  gettimeofday(&begintime, NULL);
  plcy = policy;
  init = TRUE;
  if((logfd = open("log.txt", O_CREAT | O_TRUNC, 0666)) == FAIL) {
    return FAIL;
  }  
  char titles[] = "[ticks]\tOPERATION\tTID\tPRIORITY\n";
  if(write(logfd, titles, strlen(titles)) == FAIL) {
    return FAIL;
  }

  if(policy == FIFO) {
    fqueue = new_queue();
    if(fqueue == NULL) {
      return FAIL;
    }
    curq = fqueue;
  } else if (policy == SJF) {
    squeue = new_queue();
    if(squeue == NULL) {
      return FAIL;
    }
    curq = squeue;
  } else if (policy == PRIORITY) {
    first = new_queue();
    second = new_queue();
    third = new_queue();
    if(first == NULL || second == NULL || third == NULL) {
      return FAIL;
    }
  }  
}


int thread_libterminated() {
  if(init != TRUE) {
    return FAIL;
  }
  free_queue(fqueue);
  if(close(logfd) == FAIL) {
    return FAIL;
  }
  return SUCCESS;
  //  free_tnode(curhead);
}

int thread_create(void (*func)(void*), void* arg, int priority) {
  if(plcy == FIFO) {

  }
}

int main(int argc, char** argv) {
  struct timeval begin;
  struct timeval end;
  gettimeofday(&begin, NULL);
  ucontext_t uc;
  void* stack = malloc(2048);
  uc.uc_stack.ss_sp = stack;
  uc.uc_stack.ss_size = 2048;
  getcontext(&uc);
  thrd* td = new_thread(0, uc);
  tnode* tn = new_tnode(td, NULL);
  pqueue pq = new_queue();
  insert(pq, tn);
  tnode* h = get_head(pq);
  gettimeofday(&end, NULL);
  printf("head tid is %d\n", h->td->tid);
  double interval = end.tv_sec*1000000 + end.tv_usec -begin.tv_sec*1000000 - begin.tv_usec;
  printf("execution time is %f\n", interval);
}
