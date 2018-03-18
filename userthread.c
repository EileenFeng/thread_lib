// needs to reset the three counters for priority when it's full

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

#define QUANTA 100000 // in microseconds = 100 milliseconds

#define H -1
#define M 0
#define L 1
#define HNUM 9  
#define MNUM 6 
#define LNUM 4 
#define TOTAL 19

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
static struct itimerval timer; // timer for priority scheduling
static struct itimerval store;

static int lib_init = FALSE;
static int schedule_policy = -1;    // policy of current scheduling
static int tidcount = 2; //counting the number of threads
static int firstthread = TRUE;
static double total_runtime = 0;
static int total_thrdnum = 0;

sigset_t blocked; // signal set for priority scheduling

static int logfd;        // the file descriptor of the log file
static FILE* stream = NULL;

static tnode* head = NULL; //points to the current running thread in the queue
static thrd* mainthread = NULL;
static tnode* mainnode = NULL;

static tnode* hhead;
static tnode* mhead;
static tnode* lhead;

int turns[TOTAL];
int counter = 0;

static ucontext_t* maincontext;
static ucontext_t* schedule;

static pqueue sus_queue = NULL; //suspended threads b/c joining threads
//extern void (*logfile) (double, char*, int, int);

/************************ function prototypes *****************************/
static void scheduler(int policy, int insert_sus);
static void stubfunc(void (*func)(void *), void *arg);
static void logfile(double time, const char* state, int tid, int priority);
static double calculate_time(struct timeval end, struct timeval begin);
static void sig_handler(int);
// adding and deleting node from priority queue
static void add_priority(tnode*);
static tnode* find_priority(int);
static void get_next_run();


/************************** FUNCTIONS *********************************/

static void get_next_run() {
  int priority;
  if(hhead == NULL && mhead == NULL && lhead == NULL) {
    head = NULL;
    return;
  }
  do {
    priority = turns[counter];
    if(priority == H) {
      head = hhead;
      if(hhead != NULL) {
	hhead = hhead->next;
      }
    } else if(priority == M) {
      head = mhead;
      if(mhead != NULL) {
	mhead = mhead->next;
      }
    } else if(priority == L) {
      head = lhead;
      if(lhead != NULL) {
	lhead = lhead->next;
      }
    }
    counter ++;
    counter = counter % TOTAL;
  } while(head == NULL);
}

static void add_priority(tnode* target) {
  if(target->td->priority == H) {
    insert_tail(first, target);
  } else if(target->td->priority == M) {
    insert_tail(second, target);
  } else if(target->td->priority == L) {
    insert_tail(third, target);
  }
}

static tnode* find_priority(int tid) {
  tnode* result = NULL;
  result = find_tid(first, tid);
  if(result != NULL) {
    return result;
  }
  result = find_tid(second, tid);
  if(result != NULL) {
    return result;
  }
  result = find_tid(third, tid);
  if(result != NULL) {
    return result;
  }
  return NULL;
}


static void sig_handler(int signal){
  sigprocmask(SIG_BLOCK, &blocked, NULL);  
  printf("sighandler&&&&&&&&&&&&&&&&&&& calls %d\n", head->td->tid);
  makecontext(schedule, (void (*) (void))scheduler, 2, schedule_policy, FALSE);
  swapcontext(head->td->uc, schedule);
}

static void logfile(double runtime, const char* state, int tid, int priority) {
  fprintf(stream, "[%f]\t%s\t%d\t%d\n", runtime, state, tid, priority);
}

static double calculate_time(struct timeval end, struct timeval begin) {
  double runtime = end.tv_sec*1000000 + end.tv_usec - begin.tv_sec*1000000 - begin.tv_usec;
  return runtime;
}

static void stubfunc(void (*func)(void *), void *arg) {

  if(schedule_policy == FIFO || schedule_policy == SJF) {

    struct timeval begin;
    struct timeval end;
    gettimeofday(&begin, NULL);
    gettimeofday(head->td->start, NULL);
    func(arg);
    gettimeofday(&end, NULL);
    double runtime = calculate_time(end, begin);
    // update total run time
    total_thrdnum ++;
    total_runtime += runtime;
    // update current thread's run time
    head->td->last_run = runtime;
    head->td->state = FINISHED;
    int temp = head->td->index;
    head->td->last_thr_run[temp] = runtime;
    head->td->index = (temp + 1) % RECORD_NUM;
    makecontext(schedule, (void (*) (void))scheduler, 2, schedule_policy, FALSE);
    swapcontext(head->td->uc, schedule);

  } else if(schedule_policy == PRIORITY) {

    setitimer(ITIMER_REAL, &timer, NULL);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    func(arg);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    head->td->state = FINISHED;
    makecontext(schedule, (void (*)(void))scheduler, 2, PRIORITY, FALSE);
    swapcontext(head->td->uc, schedule);
  }

}

static void scheduler(int policy, int insert_sus) {
  struct timeval t;
  // when head initially is NULL
  if(head == NULL) {
    if(firstthread == TRUE) {

      if(schedule_policy == FIFO || schedule_policy == SJF) {
        if(schedule_policy == FIFO) {
          head = get_head(fifo_queue);
        } else if (schedule_policy == SJF) {
          head = get_head(sjf_queue);
        }
        if(head == NULL) {
          printf("no threads available in ready queue yet switch back to main context\n");
          swapcontext(schedule, maincontext);
        } else {
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
            gettimeofday(head->td->start, NULL);
            printf("BBBBBBBBBBBBBBBBB Begin with %d\n", head->td->tid);
            swapcontext(schedule, head->td->uc);
          } else {
            printf("In scheduler first threads in queue is running or stopped or terminated back to main context\n");
            swapcontext(schedule, maincontext);
          }
        }

      } else if (schedule_policy == PRIORITY) {
        // very first thread to join
        hhead = get_head(first);
        mhead = get_head(second);
        lhead = get_head(third);   

        //sigprocmask(SIG_BLOCK, &blocked, NULL);
        get_next_run();
        if(head != NULL) {
          head->td->state = SCHEDULED;
          if(head->td->wait_index < 0) {
            head->td->wait_index = 0;
          }
          head->td->wait_tids[head->td->wait_index] = MAINTID;
          ///sigprocmask(SIG_UNBLOCK, &blocked, NULL);
          gettimeofday(&t, NULL);
          double curtime = calculate_time(t, begintime);
          logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
          firstthread = FALSE;
          printf("--11-----before swapping now running %d with state %d priority %f\n", head->td->tid, head->td->state, head->td->priority);
          swapcontext(schedule, head->td->uc);
        } else {
          printf("switch back to maincontext\n");
          swapcontext(schedule, maincontext);
        }
      }

    } else {
      // head is NULL, finished all scheduling
      mainthread->state = SCHEDULED;
      swapcontext(schedule, maincontext);
    }
  } else {
    // record the current tnode,
    if(schedule_policy == PRIORITY) {
      int oldtid, oldprio;
      if(head->td->state == FINISHED) {
        printf("----Finished %d with state %d priority %f\n", head->td->tid, head->td->state, head->td->priority);
        int temp_index = head->td->wait_index;
        if(temp_index >= 0) {
          for(int i = 0; i <= temp_index; i++) {
            tnode* findnode = find_tid(sus_queue, head->td->wait_tids[i]);
            if(findnode != NULL) {
              if(findnode->td->state == STOPPED) {
                thrd* newthread = copy_thread(findnode->td);
                tnode* find = new_tnode(newthread, NULL);
                find->td->state = CREATED;
                add_priority(find);
              } else {
                printf("!!!!!!!!!!!!! priority !!!!!!!!!!!!!!!!! stuff in suspended queue has wrong state %d\n", findnode->td->state);
              }
            } else {
              printf("Thread %d not found in the suspended queue\n", head->td->wait_tids[i]);
            }
          }
          head->td->wait_index = UNKNOWN;
        }
        oldtid = head->td->tid;
        oldprio = head->td->priority;
        sigprocmask(SIG_UNBLOCK, &blocked, NULL);  //here added
        gettimeofday(&t, NULL);
        double curtime;
        curtime = calculate_time(t, begintime);
        logfile(curtime, "FINISHED", oldtid, oldprio);
      } else if(head->td->state == SCHEDULED){
        // get stopped by signal handler, running time exceeds quanta
        head->td->state = FINISHED;
        thrd* newthread = copy_thread(head->td);
        tnode* newnode = new_tnode(newthread, NULL);
	newnode->td->state = STOPPED;
        add_priority(newnode);
        oldtid = head->td->tid;
        oldprio = head->td->priority;
        //delete_priority(head);
        sigprocmask(SIG_UNBLOCK, &blocked, NULL); //here addede
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "STOPPED", oldtid, oldprio);

      } else if(head->td->state == STOPPED) {
        oldtid = head->td->tid;
        oldprio = head->td->priority;
        thrd* newthread = copy_thread(head->td);
        tnode* newnode = new_tnode(newthread, NULL);
	head->td->state = FINISHED;
        if(insert_sus == TRUE) {
          insert_tail(sus_queue, newnode);
        } else {
          add_priority(newnode);
        }
        //delete_priority(head);
        sigprocmask(SIG_UNBLOCK, &blocked, NULL);   // here added


        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "STOPPED", oldtid, oldprio);
      }

      sigprocmask(SIG_BLOCK, &blocked, NULL);
      get_next_run();
      if(head != NULL) {
        head->td->state = SCHEDULED;
        ///sigprocmask(SIG_UNBLOCK, &blocked, NULL);
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
        printf("+++++before swapping now running %d with state %d priority %f\n", head->td->tid, head->td->state, head->td->priority);
        swapcontext(schedule, head->td->uc);

      } else {
        printf("Finished scheduling switch back to maincontext\n");
        swapcontext(schedule, maincontext);
      }

      // end of priority scheduling
    } else if(schedule_policy == FIFO || schedule_policy == SJF) {
      if(head->td->state == FINISHED) {
        // write log file
        gettimeofday(&t, NULL);
        double curtime;
        curtime = calculate_time(t, begintime);
        logfile(curtime, "FINISHED", head->td->tid, head->td->priority);
        printf("++==++==++==FFFFFFFFFFFFFFFFFFFFfinished %d\n", head->td->tid);
        //puts waiting threads onto ready queue
        int temp_index = head->td->wait_index;
        if(temp_index >= 0) {
          for(int i = 0; i <= temp_index; i++) {
	    tnode* findnode = find_tid(sus_queue, head->td->wait_tids[i]);
	    if(findnode != NULL) {
	      if(findnode->td->state == STOPPED) {
		thrd* newthread = copy_thread(findnode->td);
		tnode* find = new_tnode(newthread, NULL);
		find->td->state = CREATED;
		if(policy == FIFO) {
		  insert_tail(fifo_queue, find);
		} else if(policy == SJF) {
		  insert(sjf_queue, find);
		}
	      } else {
		printf("!!!!!!!!!!!!!!!!!!!!!!! error!!! stuff in suspended queue has wrong state %d!\n", findnode->td->state);
	      }
	      
	    } else {
	      printf("scheduelr not found in suspended queue thread tid not found is  %d\n", head->td->wait_tids[i]);
	      
	    }
	  }
	}
	
      } else if (head->td->state == STOPPED) {
	// log the file
	gettimeofday(&t, NULL);
	double curtime = calculate_time(t, begintime);
	double runtime = calculate_time(t, *(head->td->start));
	//	gettimeofday(head->td->start, NULL);
	logfile(curtime, "STOPPED", head->td->tid, head->td->priority);
	printf("SSSSSSSSSSSSSSSTOP: thread %d runs for %f last time\n", head->td->tid, runtime);
	// update the current thread's run time
	//double runtime = calculate_time(t, *(head->td->start));
	int temp = head->td->index;
	head->td->last_thr_run[temp] = runtime;
	head->td->index = (temp+1) % RECORD_NUM;
	total_thrdnum ++;
	total_runtime += runtime;
	int recordtimes = 0;
	double timesum = 0;
	
	for(int i = 0; i < RECORD_NUM; i++) {
	  if(head->td->last_thr_run[i] >=0) {
	    recordtimes ++;
	    timesum += head->td->last_thr_run[i];
	  }
	}
	double new_priority = (double)timesum/(double)recordtimes;
	head->td->priority = new_priority;
	tnode* tempnode = head; // after update need to rearrange the order of the queue
	if(policy == SJF) {
	  printf("in arrange\n");
	  arrange_queue(sjf_queue, tempnode);
	  printf("after arrange\n");
	}
	head->td->state = FINISHED;

	//puts to either the suspended queue or the end of queue

	thrd* newthread = copy_thread(head->td);
	printf("heng\n");
	tnode* newnode = new_tnode(newthread, NULL);
	printf("head\n");
	newnode->td->state = STOPPED;
	if(insert_sus == TRUE) {
	  insert_tail(sus_queue, newnode);
	} else {
	  if(policy == FIFO) {
	    insert_tail(fifo_queue, newnode);
	  } else if (policy == SJF) {
	    insert(sjf_queue, newnode);
	  }
	}
      }

      //swap to next context
      head = head->next;
      // when the next head is NULL
      if(head != NULL) {
	if(head->td->state == FINISHED) {
	  head = head->next;
	  if(head == NULL) {
	    firstthread = TRUE;
	    printf("head is null switched back to maincontext\n");
	    swapcontext(schedule, maincontext);
	  }
	}
	gettimeofday(&t, NULL);
	double curtime = calculate_time(t, begintime);
	logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
	gettimeofday(head->td->start, NULL);
	printf("BBBBBBBBBBBBBBBBBBbefore swapping now running %d with state %d priority %f\n", head->td->tid, head->td->state, head->td->priority);
	swapcontext(schedule, head->td->uc);
      } else {
	firstthread = TRUE;
	printf("head is null switched back to maincontext\n");
	swapcontext(schedule, maincontext);
      }
    } // end of fifo and sjf

  }
}


int thread_libinit(int policy)
{

  if( policy != FIFO && policy != SJF && policy != PRIORITY) {
    lib_init = FALSE;
    return FALSE;
  }

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
  //  sigemptyset(&(maincontext->uc_sigmask));
  maincontext->uc_link = NULL;
  mainthread = new_thread(0, maincontext);
  mainthread->tid = MAINTID;
  mainthread->state = SCHEDULED;
  printf("creting for main\n");
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
  makecontext(schedule, (void (*)(void))scheduler, 2, schedule_policy, UNKNOWN);


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
    printf("create queue for sjf\n");
    sjf_queue = new_queue();
    if(sjf_queue == NULL) {
      printf("Creating sjf queue failed\n");
      return FAIL;
    }
  } else if (policy == PRIORITY) {

    first = new_queue();
    second = new_queue();
    third = new_queue();

    // set timer
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUANTA;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100;

    store.it_interval.tv_sec = 0;
    store.it_interval.tv_usec = QUANTA;
    store.it_value.tv_sec = 0;
    store.it_value.tv_usec = 0;

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    signal(SIGALRM, sig_handler);

    int temparr[4] = {-1, 0, 1, -1};
    int temp = 1;
    while(temp <= TOTAL) {
      for(int i = 0; i < 4; i++) {
	turns[temp-1] = temparr[i];
	temp ++;
      }
      if(temp == 5) {
	turns[temp-1] = M;
	temp ++;
      } else if(temp == 10) {
	turns[temp-1] = M;
	temp ++;
      } else if(temp == 15) {
	turns[temp-1] = H;
	temp++;
      }
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

  if(schedule_policy == FIFO) {
    free_queue(fifo_queue);
  } else if(schedule_policy == SJF) {
    free_queue(sjf_queue);
  } else if(schedule_policy == PRIORITY) {
    free_queue(first);
    free_queue(second);
    free_queue(third);
  }

  free_queue(sus_queue);
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

  if(schedule_policy == FIFO || schedule_policy == SJF) {
    newthread->schedule = schedule_policy;
    newthread->state = CREATED;
    newthread->last_run = 0;
    if(total_thrdnum == 0) {
      int quanta = QUANTA;
      newthread->priority = (double)(quanta/2);
    } else {
      newthread->priority = (double)total_runtime/total_thrdnum;
    }
    newnode = new_tnode(newthread, NULL);
    if(newnode == NULL) {
      return FAIL;
    }
    if(schedule_policy == FIFO) {
      insert_tail(fifo_queue, newnode);
    } else if(schedule_policy == SJF) {
      insert(sjf_queue, newnode);
    }
  } else if (schedule_policy == PRIORITY) {

    newthread->schedule = schedule_policy;
    newthread->state = CREATED;
    newthread->last_run = 0;
    newthread->priority = priority;
    newnode = new_tnode(newthread, NULL);
    if(newnode == NULL) {
      return FAIL;
    }
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    add_priority(newnode);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
  } else {
    return FAIL;
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
    makecontext(schedule, (void (*)(void))scheduler, 2, FIFO, FALSE);
    swapcontext(maincontext, schedule);
  } else {
    if(schedule_policy == FIFO || schedule_policy == SJF) {
      tnode* target;
      if(schedule_policy == FIFO) {
        target = find_tid(fifo_queue, tid);
      } else if(schedule_policy == SJF) {
        target = find_tid(sjf_queue, tid);
      }
      if(target == NULL) {
        return FAIL;
      } else if(target->td->state == FINISHED) {
        return SUCCESS;
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
      makecontext(schedule, (void (*)(void))scheduler, 2, FIFO, TRUE);
      gettimeofday(head->td->start, NULL);
      swapcontext(head->td->uc, schedule);
      return TRUE;
    } else if(schedule_policy == PRIORITY) {
      tnode* target = find_priority(tid);
      if(target == NULL) {
        return FAIL;
      } else if(target->td->state == FINISHED) {
        return SUCCESS;
      }
      if(target->td->wait_index < 0) {
        target->td->wait_index = 0;
      }  else {
        target->td->wait_index ++;
      }
      if(target->td->wait_index >= target->td->wait_size) {
        target->td->wait_size += ARRSIZE;
        target->td->wait_tids = realloc(target->td->wait_tids, sizeof(int) * target->td->wait_size);
      }
      target->td->wait_tids[target->td->wait_index] = head->td->tid;
      head->td->state = STOPPED;
      makecontext(schedule, (void (*)(void))scheduler, 2, PRIORITY, TRUE);
      swapcontext(head->td->uc, schedule);
      return TRUE;
    }

  }
  return TRUE;
}

int thread_yield(void) {

  head->td->state = STOPPED;
  printf("tid %d yield\n", head->td->tid);
  makecontext(schedule, (void (*) (void))scheduler, 2, schedule_policy, FALSE);
  swapcontext(head->td->uc, schedule);
  return SUCCESS;

}
