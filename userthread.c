/* updates:
1. need to keep track of the main running threads, which will be suspended when
joining the first thread in FIFO and SJF
2. maintains an array of tids that has joined this thread 
3. maintains a suspended queue of suspended threads because of joining (checked)
4. update the stub function for this joining feature 
(put tnodes with the corresponding tids from the susq to the ready queue) (checked)
*/

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
enum state
{
  CREATED,
  SCHEDULED,
  STOPPED,
  FINISHED
};

// priority queue for FIFO and sjf
static pqueue fqueue, squeue;

//priority queues for priority scheduling
static pqueue first, second, third;

//static timeval begintime; // records beginning time of lib init
static struct timeval begintime;
static int init = FALSE;
static int plcy = -1;    // policy of current scheduling
static int tidcount = 2; //counting the number of threads
static int logfd;        // the file descriptor of the log file
static FILE* stream = NULL;
static int firstthread = TRUE;
static ucontext_t curcontext;
//static tnode *mnode;
static pqueue susq = NULL; //suspended threads b/c joining threads

//static tnode* curhead = NULL;
//static pqueue curq = NULL; // keep track of the current queue (for fifo and sjf)

/************************ function prototypes *****************************/
static void scheduler(int policy, int insertHead);
static void stubfunc(void (*func)(void *), void *arg);

static void stubfunc(void (*func)(void *), void *arg)
{
  struct timeval begin;
  struct timeval end;
  tnode* curhead;
  if (plcy == FIFO)
  {
    curhead = get_head(fqueue);
    func(arg);
    printf("s1\n");
    curhead->td->state = FINISHED;
    // un-suspend all joining threads
    printf("wait index is %d\n",curhead->td->wait_index);
    if(curhead->td->wait_index >= 0) {
      printf("in updating\n");
      for (int i = 0; i <= curhead->td->wait_index; i++)
	{
	  printf("s2\n");
	  tnode *temp = find_tid(susq, curhead->td->wait_tids[i]);
	  printf("s3\n");
	  insert_tail(fqueue, temp);
	  printf("s4\n");
	  curhead->td->wait_tids[i] = -1;
	}
      curhead->td->wait_index = NOTSET; // reset the index
    }

    // needs to be modified
  }
  else if (plcy == SJF)
  {
    curhead = get_head(squeue);
    gettimeofday(&begin, NULL);
    func(arg);
    gettimeofday(&end, NULL);
    curhead->td->state = FINISHED;
    curhead->td->last_run = end.tv_sec * 1000000 + end.tv_usec - begin.tv_sec * 1000000 - begin.tv_usec;
    int index = curhead->td->index;
    curhead->td->last_thr_run[index] = curhead->td->last_run;
    curhead->td->index = (index + 1) % RECORD_NUM;
  }
  scheduler(plcy, FALSE);
}

static void scheduler(int policy, int insertHead)
{
  printf("in scheduler\n");
  struct timeval t;
  double curtime;
  if (policy == FIFO)
  {

    if (get_size(fqueue) == 0)
    {
      printf("fifo queue is emtpy\n");
      return;
    }

    if (firstthread == TRUE)
    {
      printf("first thread\n");
      tnode *firstt = get_head(fqueue);
      if (firstt != NULL)
      {
	printf("first run: tid is : %d\n", firstt->td->tid);
        firstt->td->state = SCHEDULED;
        firstthread = FALSE;
        gettimeofday(&t, NULL);
        curtime = t.tv_sec * 1000000 + t.tv_usec - begintime.tv_sec * 1000000 - begintime.tv_usec;
        fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "SCHEDULED", firstt->td->tid, UNKNOWN);
        swapcontext(&curcontext, &(firstt->td->uc));
      }
      return;
    }

    tnode *oldhead = get_head(fqueue);
    //int oheadtid = oldhead->td->tid;
    tnode *newhead = NULL;
    
    if (oldhead->td->state != FINISHED)
    {
      pop_head(fqueue);
      newhead = get_head(fqueue);
      if(insertHead) {
        printf("inserting old head to the end of queue in scheduler\n");
        insert_tail(fqueue, oldhead);
      }
      if(newhead != NULL) {
        newhead->td->state = SCHEDULED;
        gettimeofday(&t, NULL);
        curtime = t.tv_sec * 1000000 + t.tv_usec - begintime.tv_sec * 1000000 - begintime.tv_usec;
        fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "SCHEDULED", newhead->td->tid, UNKNOWN);
	swapcontext(&(oldhead->td->uc), &(newhead->td->uc));
      } 
      return;   
    }
    else
    {
      printf("finished schduling get size %d\n", get_size(fqueue)); 
      delete_head(fqueue);
      printf("deleted head\n");
      newhead = get_head(fqueue);
      if(newhead != NULL) {
	printf("new head not null in scheduler\n");
	newhead->td->state = SCHEDULED;
        gettimeofday(&t, NULL);
	curtime = t.tv_sec * 1000000 + t.tv_usec - begintime.tv_sec * 1000000 - begintime.tv_usec;
        fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "SCHEDULED", newhead->td->tid, UNKNOWN);
        setcontext(&(newhead->td->uc));
      } else {
	return;
      }
      return;
    }


    
  }
  else
  {
    printf("not yet implemented\n");
  }
}


int thread_libinit(int policy)
{
  // store the current context
  susq = new_queue();
  plcy = policy;
  init = TRUE;
  gettimeofday(&begintime, NULL);
  void *stack = malloc(STACKSIZE);
  curcontext.uc_stack.ss_sp = stack;
  curcontext.uc_stack.ss_size = STACKSIZE;
  curcontext.uc_link = NULL;
  curcontext.uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&curcontext.uc_sigmask);
  getcontext(&curcontext);
  if ((logfd = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, 0666)) == FAIL)
  {
    printf("open failed\n");
    return FAIL;
  }
  if((stream = fdopen(logfd, "w")) == NULL) {
    perror("Failed:");
    return FAIL;
  }
  char titles[] = "[ticks]\tOPERATION\tTID\tPRIORITY\n";
  if (write(logfd, titles, strlen(titles)) == FAIL)
  {
    printf("write failed\n");
    return FAIL;
  }

  if (policy == FIFO)
  {
    printf("3\n");
    fqueue = new_queue();
    if (fqueue == NULL)
    {
      return FAIL;
    }
  }
  else if (policy == SJF)
  {
    squeue = new_queue();
    if (squeue == NULL)
    {
      return FAIL;
    }
  }
  else if (policy == PRIORITY)
  {
    first = new_queue();
    second = new_queue();
    third = new_queue();
    if (first == NULL || second == NULL || third == NULL)
    {
      return FAIL;
    }
  }
  return SUCCESS;
}

int thread_libterminate(void)
{ 
  printf("call here\n");
  if (init != TRUE) { printf("haha\n"); return FAIL; }
  
  if (plcy == FIFO)
  {
    free_queue(fqueue);
  }
  free_queue(susq);
  printf("free here\n");
  free(curcontext.uc_stack.ss_sp);
  
  //if (close(logfd) == FAIL) { perror("close");return FAIL; }
  if(fclose(stream) == EOF) { printf("wata\n");return FAIL;}     
  return SUCCESS;
  //  free_tnode(curhead);
}

int thread_create(void (*func)(void *), void *arg, int priority)
{
  if (plcy == FIFO)
  {
    ucontext_t uc;
    getcontext(&uc);
    void *stack = (void*)malloc(STACKSIZE);
    uc.uc_stack.ss_sp = stack;
    uc.uc_stack.ss_size = STACKSIZE;
    uc.uc_link = NULL;
    uc.uc_stack.ss_flags = SS_DISABLE;
    sigemptyset(&uc.uc_sigmask);
    makecontext(&uc, (void (*)(void))stubfunc, 2, func, arg);

    thrd *td = new_thread(tidcount, uc);
    tidcount++;
    td->schedule = FIFO;
    td->state = CREATED;
    td->last_run = 0;
    tnode *tn = new_tnode(td, NULL);
    // put on queue

    insert_tail(fqueue, tn);
    printf("in created the tid of the head is %d\n", sizeof(get_head(fqueue)->td->uc.uc_stack.ss_sp));
    double curtime = begintime.tv_sec * 1000000 + begintime.tv_usec;
    fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "CREATED", td->tid, UNKNOWN);
    return tn->td->tid;
  }
  return FAIL;

}

int thread_join(int tid)
{
  struct timeval t;
  double curtime;

  if (plcy == FIFO || plcy == SJF)
  {
    if (firstthread == TRUE)
    {
      scheduler(plcy, FALSE);
      return SUCCESS;
    }

    tnode *thead = NULL;
    tnode *target = NULL;


    if (plcy == FIFO)
    {
      thead = get_head(fqueue);
      target = find_tid(fqueue, tid);

      if (target == NULL)
      {
        return FAIL;
      }
      if(target->td->wait_index < 0) {
	target->td->wait_index = 0;
      }
      if(target->td->wait_index >= target->td->wait_size) {
        target->td->wait_size += ARRSIZE;
        target->td->wait_tids = realloc(target->td->wait_tids, sizeof(int) * target->td->wait_size);
      }
      target->td->wait_tids[target->td->wait_index] = thead->td->tid;
      target->td->wait_index++;
      // put thead to suspended queue
      insert_tail(susq, thead);
      thead->td->state = STOPPED;
      gettimeofday(&t, NULL);
      curtime = t.tv_sec * 1000000 + t.tv_usec - begintime.tv_sec * 1000000 - begintime.tv_usec;
      fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "STOPPED", thead->td->tid, UNKNOWN);
      scheduler(FIFO, FALSE);
      return SUCCESS;
    }


  }
  return FAIL;
}

int thread_yield(void) {
  tnode* curhead;
  struct timeval t;
  double curtime;

  if(plcy == FIFO) {
    curhead = get_head(fqueue);
    curhead->td->state = STOPPED;
    gettimeofday(&t, NULL);
    curtime = t.tv_sec * 1000000 + t.tv_usec - begintime.tv_sec * 1000000 - begintime.tv_usec;
    fprintf(stream, "[%f]\t%s\t%d\t%d\n", curtime, "STOPPED", curhead->td->tid, UNKNOWN);
    scheduler(FIFO, TRUE);
    return SUCCESS;
  }
  return FAIL;
}

/*
int main(int argc, char **argv)
{
  thread_libinit(FIFO);
  //  thread_libterminate();
  struct timeval begin;
  struct timeval end;
  gettimeofday(&begin, NULL);
  ucontext_t uc;
  void *stack = malloc(2048);
  uc.uc_stack.ss_sp = stack;
  uc.uc_stack.ss_size = 2048;
  getcontext(&uc);
  thrd *td = new_thread(0, uc);
  tnode *tn = new_tnode(td, NULL);
  pqueue pq = new_queue();
  insert(pq, tn);
  tnode *h = get_head(pq);
  gettimeofday(&end, NULL);
  printf("head tid is %d\n", h->td->tid);
  double interval = end.tv_sec * 1000000 + end.tv_usec - begin.tv_sec * 1000000 - begin.tv_usec;
  printf("execution time is %f\n", interval);
  
}
*/
