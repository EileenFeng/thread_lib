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
#define MAINTID 1

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
static pqueue fifo_queue, sjf_queue;

//priority queues for priority scheduling
static pqueue first, second, third;

//static timeval begintime; // records beginning time of lib init
static struct timeval begintime;
static int lib_init = FALSE;
static int schedule_policy = -1;    // policy of current scheduling
static int tidcount = 2; //counting the number of threads
static int firstthread = TRUE;

static int logfd;        // the file descriptor of the log file
static FILE* stream = NULL;

static tnode* head = NULL; //points to the current running thread in the queue
static thrd* mainthread = NULL;
static tnode* mainnode = NULL;

static ucontext_t* maincontext;
static ucontext_t* schedule; 

static pqueue sus_queue = NULL; //suspended threads b/c joining threads
//extern void (*logfile) (double, char*, int, int); 

/************************ function prototypes *****************************/
static void scheduler(int policy, int insert_sus);
static void stubfunc(void (*func)(void *), void *arg);
static void log_file(double time, const char* state, int tid, int priority);
static double calculate_time(struct timeval end, struct timeval begin);

static void log_file(double runtime, const char* state, int tid, int priority) {
  fprintf(stream, "[%f]\t%s\t%d\t%d\n", runtime, state, tid, priority);
}

double (*logfile) (double, char*, int, int) = &log_file;


static double calculate_time(struct timeval end, struct timeval begin) {
  double runtime = end.tv_sec*1000000 + end.tv_usec - begin.tv_sec*1000000 - begin.tv_usec;
  return runtime;
}

static void stubfunc(void (*func)(void *), void *arg) {  

  struct timeval begin;
  struct timeval end;
  gettimeofday(&begin, NULL);
  func(arg);
  gettimeofday(&end, NULL);   
  double runtime = calculate_time(end, begin);
  head->td->last_run = runtime;
  head->td->state = FINISHED;
  //make context for schedule context -> yes need to update insert_sus and arguments passed in
  makecontext(schedule, scheduler, 2, schedule_policy, FALSE);
  swapcontext(head->td->uc, schedule);

}

static void scheduler(int policy, int insert_sus) {
  struct timeval t;

  // when head initially is NULL
  if(head == NULL) {
    if(firstthread == TRUE) {

      if(schedule_policy == FIFO) {
	head = get_head(fifo_queue);
	if(head->td->state == CREATED) {
	  head->td->state = SCHEDULED;
	  gettimeofday(&t, NULL);
	  double curtime = calculate_time(t, begintime);
	  logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
	  firstthread = FALSE;
	  if(head->td->wait_index < 0) {
	    head->td->wait_index = 0;
	  }
	  head->td->wait_tids[head->td->wait_index] = MAINTID;
	  swapcontext(schedule, head->td->uc);
	} else {
	  printf("In scheduler first threads in queue is running or stopped or terminated\n");
	}
      }



    } else {
      // head is NULL, finished all scheduling
      mainthread->state = SCHEDULED;
      swapcontext(schedule, maincontext);
    }
  } else {
    // record the current tnode,

    if(schedule_policy == FIFO) {
      if(head->td->state == FINISHED) {
	// write log file
	gettimeofday(&t, NULL);
	double curtime;
	curtime = calculate_time(t, begintime);
	logfile(curtime, "FINISHED", head->td->tid, head->td->priority);
                
	//puts waiting threads onto ready queue               
	int temp_index = head->td->wait_index;
	if(temp_index >= 0) {
	  for(int i = 0; i <= temp_index; i++) {
	    /* if(head->td->wait_tids[i] == MAINTID) {
	      insert_tail(fifo_queue, mainnode);
	      printf("added mainnode to fifo_queue\n");
	      } else {*/
	    
	    tnode* find = find_tid(sus_queue, head->td->wait_tids[i]);
	    if(find != NULL) {
	      printf("inserting back to READDDDDDY queue %d\n", find->td->tid);
	      insert_tail(fifo_queue, find);
	    } else {
	      printf("scheduelr not found in suspended queue error\n");
	    }

	      // }
	  }
	}
	
      } else if (head->td->state == STOPPED) {
	// log the file
	gettimeofday(&t, NULL);
	double curtime = calculate_time(t, begintime);
	logfile(curtime, "STOPPED", head->td->tid, head->td->priority);

	//puts to either the suspended queue or the end of queue
	if(insert_sus == TRUE) {
	  printf("Scheduler in inserting susqueue %d\n", head->td->tid);
	  thrd* newthread = copy_thread(head->td);                                                    tnode* newnode = new_tnode(newthread, NULL); 
	  insert_tail(sus_queue, newnode);
	  printf("after insertion to sus queue %d\n", newnode->td->tid);
	} else {
	  thrd* newthread = copy_thread(head->td);
	  tnode* newnode = new_tnode(newthread, NULL);
	  insert_tail(fifo_queue, newnode);
	}
      }

      //swap to next context
      head = head->next;
      // when the next head is NULL
      if(head != NULL) {
	printf("Scheduler swap to next context with tid %d\n", head->td->tid);
	gettimeofday(&t, NULL);
	double curtime = calculate_time(t, begintime);
	logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
	swapcontext(schedule, head->td->uc);
      } else {
	printf("No threads available Scheduler swap to main context\n");
	swapcontext(schedule, maincontext);
      }
    } else {
      printf("Not yet implemented\n");
    } 


  }
}


int thread_libinit(int policy)
{

  sus_queue = new_queue();
  schedule_policy = policy;
  gettimeofday(&begintime, NULL);

  void* stack;
  maincontext = malloc(sizeof(ucontext_t));
  getcontext(maincontext);
  stack = malloc(STACKSIZE);
  maincontext->uc_stack.ss_sp = stack;
  maincontext->uc_stack.ss_size = STACKSIZE;
  maincontext->uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&(maincontext->uc_sigmask));
  maincontext->uc_link = NULL;
  mainthread = new_thread(0, maincontext);
  mainthread->tid = MAINTID;
  mainthread->state = SCHEDULED;
  mainnode = new_tnode(mainthread, NULL);
  
  void* sta;
  schedule = malloc(sizeof(ucontext_t));
  getcontext(schedule);
  sta = malloc(STACKSIZE);
  schedule->uc_stack.ss_sp = sta;
  schedule->uc_stack.ss_size = STACKSIZE;
  schedule->uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&(schedule->uc_sigmask));
  schedule->uc_link = NULL;
  makecontext(schedule, scheduler, 2, schedule_policy, UNKNOWN);
  
  if((logfd = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, 0666)) == FAIL) {
    perror("Failed to open file in init: ");
    return FAIL;
  }

  if((stream = fdopen(logfd, "w")) == NULL) {
    perror("Failed to open stream in init: ");
    return FAIL;
  }
    
  char titles[] = "[ticks]\tOPERATION\tTID\tPRIORITY\n";
  if (write(logfd, titles, strlen(titles)) == FAIL) {
    perror("write failed in init: ");
    return FAIL;
  }

  // initilize queues
  if(policy == FIFO) {
    fifo_queue = new_queue();
    if(fifo_queue == NULL) {
      printf("Creating fifo queue failed\n");
      return FAIL;
    }
  } else if (policy == SJF) {
    sjf_queue = new_queue();
    if(sjf_queue == NULL) {
      printf("Creating sjf queue failed\n");
      return FAIL;
    }
  }
  lib_init = TRUE;

  return SUCCESS;
}


int thread_libterminate(void)
{
  if(lib_init != TRUE) {
    printf("library not initialized\n");
    return FAIL;
  }
  printf("fifo_queue size %d\n", get_size(fifo_queue));
  
  if(schedule_policy == FIFO) {
    free_queue(fifo_queue);
  } else if(schedule_policy == SJF) {
    free_queue(sjf_queue);
  }
  printf("susq %d\n", get_size(sus_queue));
  free_queue(sus_queue);
  printf("after susq\n");
  free(maincontext->uc_stack.ss_sp);
  free(maincontext);
  free(mainthread->wait_tids);
  free(mainthread->start);
  free(mainthread);
  free(mainnode);
  free(schedule->uc_stack.ss_sp);
  free(schedule);
  if(fclose(stream) == EOF) {
    perror("Close stream in terminate failed: ");
    return FAIL;
  }
  return SUCCESS;
}


int thread_create(void (*func)(void *), void *arg, int priority)
{
  struct timeval t;
  ucontext_t* newuc = malloc(sizeof(ucontext_t));
  void *stack;
  getcontext(newuc);
  stack = malloc(STACKSIZE);
  newuc->uc_stack.ss_sp = stack;
  newuc->uc_stack.ss_size = STACKSIZE;
  newuc->uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&(newuc->uc_sigmask));
  makecontext(newuc, (void (*)(void))stubfunc, 2, func, arg);
  thrd* newthread = new_thread(tidcount, newuc);
  if(newthread == NULL) {
    return FAIL;
  }
  tidcount++;
  tnode* newnode = NULL;
  if(schedule_policy == FIFO) {
    newthread->schedule = FIFO;
    newthread->state = CREATED;
    newthread->last_run = 0;
    newthread->priority = UNKNOWN;

    newnode = new_tnode(newthread, NULL);
    if(newnode == NULL) {
      return FAIL;
    }
    insert_tail(fifo_queue, newnode);
  }
  gettimeofday(&t, NULL);
  double curtime = calculate_time(t, begintime);
  logfile(curtime, "CREATED", newthread->tid, newthread->priority);
  return newnode->td->tid;
}


int thread_join(int tid) {

  if(head == NULL) {
    // the main thread is waiting for the thread tid
    mainthread->state = STOPPED;
    mainthread->wait_index = 0;
    mainthread->wait_tids[mainthread->wait_index] = tid;
    makecontext(schedule, scheduler, 2, FIFO, FALSE);
    swapcontext(maincontext, schedule);
  } else {
    // wait_tid incremented before adding new stuff, thus the current tid will always points to some tid
    // check scheduler wait_tid for more info

    if(schedule_policy == FIFO) {
      tnode* target = find_tid(fifo_queue, tid);
      if(target == NULL) {
	return FAIL;
      } else if(target->td->state == FINISHED) {
	return FAIL;
      }
      if(target->td->wait_index < 0) {
	target->td->wait_index = 0;
      } else {
	target->td->wait_index ++;
      }
      if(target->td->wait_index >= target->td->wait_size) {
	target->td->wait_size += ARRSIZE;
	target->td->wait_tids = realloc(target->td->wait_tids, sizeof(int) * target->td->wait_size);
      }
      target->td->wait_tids[target->td->wait_index] = head->td->tid;
      head->td->state = STOPPED;
      makecontext(schedule, scheduler, 2, FIFO, TRUE);
      swapcontext(head->td->uc, schedule);
      return TRUE;
    }
  }
  return TRUE;
}

int thread_yield(void) {
    
  head->td->state = STOPPED;
  makecontext(schedule, scheduler, 2, schedule_policy, FALSE);
  swapcontext(head->td->uc, schedule);

}


 
