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
#include <sys/mman.h>
#include "/usr/include/valgrind/valgrind.h"
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

static pqueue finish_list;
//static timeval begintime; // records beginning time of lib init
static struct timeval begintime;
static struct itimerval timer; // timer for priority scheduling

static int lib_init = FALSE;
static int lib_term = FALSE;
static int schedule_policy = -1;    // policy of current scheduling
static int tidcount = 2; //counting the number of threads
static int firstthread = TRUE;
static double total_runtime = 0;
static int total_thrdnum = 0;
static int schedule_id;

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
static void pop_priority();
static void reset_heads_priority();
static void free_basics();
static int update_check_ready(tnode*, int);

/************************** FUNCTIONS *********************************/

static int update_check_ready(tnode* target, int tid) {
  int ready = TRUE;
  for(int i = 0; i <= target->td->waiting_index; i++) {
    if(target->td->waiting[i] == tid) {
      target->td->waiting[i] = UNKNOWN;
    }
    if(target->td->waiting[i] >= 0) {
      ready = FALSE;
    }
  }
  return ready;
}

static void reset_heads_priority() {
  if(head->td->priority == H) {
    hhead = get_head(first);
  }else if(head->td->priority == M) {
    mhead = get_head(second);
  } else if(head->td->priority == L) {
    lhead = get_head(third);
  }
}

static void free_basics(){
  VALGRIND_STACK_DEREGISTER(mainthread->valgrindid);
  free(maincontext->uc_stack.ss_sp);
  free(maincontext);
  free(mainthread->wait_tids);
  free(mainthread->start);
  free(mainthread);
  free(mainnode);
  VALGRIND_STACK_DEREGISTER(schedule_id);
  free(schedule->uc_stack.ss_sp);
  free(schedule);
}

static void pop_priority() {
  //  tnode* result = NULL;
  if(head->td->priority == H) {
    pop_head(first);
  } else if (head->td->priority == M) {
    pop_head(second);
  } else if(head->td->priority == L) {
    pop_head(third);
  }
}

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

  if(policy == PRIORITY) {
    sigprocmask(SIG_BLOCK, &blocked, NULL);
  }
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
            swapcontext(schedule, head->td->uc);
          } else {
            swapcontext(schedule, maincontext);
          }
        }

      } else if (schedule_policy == PRIORITY) {
        // very first thread to join
        hhead = get_head(first);
        mhead = get_head(second);
        lhead = get_head(third);

        get_next_run();
        if(head != NULL) {
          head->td->state = SCHEDULED;
          if(head->td->wait_index < 0) {
            head->td->wait_index = 0;
          }
          head->td->wait_tids[head->td->wait_index] = MAINTID;
          gettimeofday(&t, NULL);
          double curtime = calculate_time(t, begintime);
          logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
          firstthread = FALSE;
          swapcontext(schedule, head->td->uc);
        } else {
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
      int resethead = FALSE;
      if(head->td->state == FINISHED) {
        pop_priority(head);
        if(head->td->priority == H) {
          resethead = get_size(first) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == M) {
          resethead = get_size(second) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == L) {
          resethead = get_size(third) == 0 ? TRUE : FALSE;
        }

        int temp_index = head->td->wait_index;
        if(temp_index >= 0) {
          for(int i = 0; i <= temp_index; i++) {
            tnode* find = find_tid(sus_queue, head->td->wait_tids[i]);
            if(find != NULL) {
              int ready = update_check_ready(find, head->td->tid);
              if(find->td->state == STOPPED && ready) {
                pop_node(sus_queue, find);
                find->td->state = CREATED;
                add_priority(find);
              }
            }
          }
          head->td->wait_index = UNKNOWN;
        }

        oldtid = head->td->tid;
        oldprio = head->td->priority;
        gettimeofday(&t, NULL);
        double curtime;
        curtime = calculate_time(t, begintime);
        logfile(curtime, "FINISHED", oldtid, oldprio);
        if(resethead == TRUE) {
          reset_heads_priority();
        }
        if(head->td->tid != mainnode->td->tid) {
          head->next = NULL;
          insert_tail(finish_list, head);
        }
        head = NULL;
      } else if(head->td->state == SCHEDULED){
        // get stopped by signal handler, running time exceeds quanta
        head->td->state = STOPPED;
        pop_priority(head);
        if(head->td->priority == H) {
          resethead = get_size(first) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == M) {
          resethead = get_size(second) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == L) {
          resethead = get_size(third) == 0 ? TRUE : FALSE;
        }

        oldtid = head->td->tid;
        oldprio = head->td->priority;
        add_priority(head);
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "STOPPED", oldtid, oldprio);
        if(resethead == TRUE) {
          reset_heads_priority();
        }

      } else if(head->td->state == STOPPED) {
        oldtid = head->td->tid;
        oldprio = head->td->priority;

        pop_priority(head);
        if(head->td->priority == H) {
          resethead = get_size(first) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == M) {
          resethead = get_size(second) == 0 ? TRUE : FALSE;
        } else if (head->td->priority == L) {
          resethead = get_size(third) == 0 ? TRUE : FALSE;
        }

        if(insert_sus == TRUE) {
          insert_tail(sus_queue, head);
        } else {
          add_priority(head);
        }
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "STOPPED", oldtid, oldprio);
        if(resethead == TRUE) {
          reset_heads_priority();
        }
      }

      get_next_run();
      if(head != NULL) {
        head->td->state = SCHEDULED;
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
        setitimer(ITIMER_REAL, &timer, NULL);
        swapcontext(schedule, head->td->uc);
      } else {
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

        if(policy == FIFO) {
          pop_head(fifo_queue);
        } else if (policy == SJF) {
          pop_head(sjf_queue);
        }
        head->next = NULL;
        //puts waiting threads onto ready queue
        int temp_index = head->td->wait_index;
        if(temp_index >= 0) {
          for(int i = 0; i <= temp_index; i++) {
            tnode* findnode = find_tid(sus_queue, head->td->wait_tids[i]);
            if(findnode != NULL) {
              int ready = update_check_ready(findnode, head->td->tid);
              if(findnode->td->state == STOPPED && ready) {
                pop_node(sus_queue, findnode);
                findnode->td->state = CREATED;
                if(policy == FIFO) {
                  insert_tail(fifo_queue, findnode);
                } else if(policy == SJF) {
                  insert(sjf_queue, findnode);
                }
              }
            }
          }
        }
        insert_tail(finish_list, head);
        head = NULL;
      } else if (head->td->state == STOPPED) {
        // log the file
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        double runtime = calculate_time(t, *(head->td->start));
        logfile(curtime, "STOPPED", head->td->tid, head->td->priority);
        // update the current thread's run time
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

        if(policy == FIFO) {
          pop_head(fifo_queue);
        } else if(policy == SJF) {
          pop_head(sjf_queue);
        }
        head->next = NULL;
        if(insert_sus == TRUE) {
          insert_tail(sus_queue, head);
        } else {
          if(policy == FIFO) {
            insert_tail(fifo_queue, head);
          } else if (policy == SJF) {
            insert(sjf_queue, head);
          }
        }
      }
      if(policy == FIFO) {
        head = get_head(fifo_queue);
      } else if(policy == SJF) {
        head = get_head(sjf_queue);
      }
      if(head != NULL) {
        if(head->td->state == FINISHED) {
          head = head->next;
          if(head == NULL) {
            firstthread = TRUE;
            swapcontext(schedule, maincontext);
          }
        }
        gettimeofday(&t, NULL);
        double curtime = calculate_time(t, begintime);
        logfile(curtime, "SCHEDULED", head->td->tid, head->td->priority);
        gettimeofday(head->td->start, NULL);
        swapcontext(schedule, head->td->uc);
      } else {
        firstthread = TRUE;
        swapcontext(schedule, maincontext);
      }
    } // end of fifo and sjf
  }
}


int thread_libinit(int policy)
{

  if( policy != FIFO && policy != SJF && policy != PRIORITY) {
    lib_init = FALSE;
    return FAIL;
  }

  sus_queue = new_queue();
  schedule_policy = policy;
  gettimeofday(&begintime, NULL);

  void* stack;
  maincontext = malloc(sizeof(ucontext_t));
  mainthread = new_thread(0, maincontext);
  getcontext(maincontext);
  stack = malloc(STACKSIZE);
  mainthread->valgrindid = VALGRIND_STACK_REGISTER(stack, stack+STACKSIZE);
  maincontext->uc_stack.ss_sp = stack;
  maincontext->uc_stack.ss_size = STACKSIZE;
  maincontext->uc_stack.ss_flags = SS_DISABLE;
  maincontext->uc_link = NULL;
  mainthread->tid = MAINTID;
  mainthread->state = SCHEDULED;
  mainnode = new_tnode(mainthread, NULL);

  void* sta;
  schedule = malloc(sizeof(ucontext_t));
  getcontext(schedule);
  sta = malloc(STACKSIZE);
  schedule_id = VALGRIND_STACK_REGISTER(sta, sta+STACKSIZE);
  schedule->uc_stack.ss_sp = sta;
  schedule->uc_stack.ss_size = STACKSIZE;
  schedule->uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&(schedule->uc_sigmask));
  schedule->uc_link = NULL;
  makecontext(schedule, (void (*)(void))scheduler, 2, schedule_policy, UNKNOWN);

  if((logfd = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, 0666)) == FAIL) {
    free_basics();
    return FAIL;
  }

  if((stream = fdopen(logfd, "w")) == NULL) {
    free_basics();
    return FAIL;
  }

  char titles[] = "[ticks]\tOPERATION\tTID\tPRIORITY\n";
  if (write(logfd, titles, strlen(titles)) == FAIL) {
    free_basics();
    return FAIL;
  }

  if(policy == FIFO) {
    fifo_queue = new_queue();
    if(fifo_queue == NULL) {
      free_basics();
      return FAIL;
    }
  } else if (policy == SJF) {
    sjf_queue = new_queue();
    if(sjf_queue == NULL) {
      free_basics();
      return FAIL;
    }
  } else if (policy == PRIORITY) {
    first = new_queue();
    if(first == NULL) {
      free_basics();
      return FAIL;
    }
    second = new_queue();
    if(second == NULL) {
      free_basics();
      free(first);
      return FAIL;
    }
    third = new_queue();
    if(third == NULL) {
      free_basics();
      free(second);
      free(third);
      return FAIL;
    }
    // set timer
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUANTA;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = QUANTA;

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
  finish_list = new_queue();
  lib_init = TRUE;
  return SUCCESS;
}


int thread_libterminate(void)
{
  if(lib_init != TRUE || lib_term == TRUE) {
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
  free_queue(finish_list);
  //VALGRIND_STACK_DEREGISTER(mainthread->valgrindid);
  /*free(maincontext->uc_stack.ss_sp);
  free(maincontext);
  free(mainthread->wait_tids);
  free(mainthread->start);
  free(mainthread);
  free(mainnode);
  */
  VALGRIND_STACK_DEREGISTER(schedule_id);
  free(schedule->uc_stack.ss_sp);
  free(schedule);
  if(fclose(stream) == EOF) {
    return FAIL;
  }
  lib_term = TRUE;
  return SUCCESS;
}


int thread_create(void (*func)(void *), void *arg, int priority)
{
  if(lib_init != TRUE || lib_term == TRUE) {
    return FAIL;
  }
  struct timeval t;
  ucontext_t* newuc = malloc(sizeof(ucontext_t));
  thrd* newthread = new_thread(tidcount, newuc);
  void *stack;
  getcontext(newuc);
  stack = malloc(STACKSIZE);
  newthread->valgrindid = VALGRIND_STACK_REGISTER(stack, stack+STACKSIZE);
  newuc->uc_stack.ss_sp = stack;
  newuc->uc_stack.ss_size = STACKSIZE;
  newuc->uc_stack.ss_flags = SS_DISABLE;
  sigemptyset(&(newuc->uc_sigmask));
  makecontext(newuc, (void (*)(void))stubfunc, 2, func, arg);
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
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
  } else {
    return FAIL;
  }
  gettimeofday(&t, NULL);
  double curtime = calculate_time(t, begintime);
  logfile(curtime, "CREATED", newthread->tid, newthread->priority);
  return newnode->td->tid;
}


int thread_join(int tid) {

  if(lib_init != TRUE || lib_term == TRUE) {
    return FAIL;
  }

  if(head == NULL) {
    // the main thread is waiting for the thread tid
    tnode* target;
    if(schedule_policy == PRIORITY) {
      target = find_priority(tid);
    } else if(schedule_policy == SJF) {
      target = find_tid(sjf_queue, tid);
    } else if(schedule_policy == FIFO) {
      target = find_tid(fifo_queue, tid);
    }
    if(target == NULL) {
      target = find_tid(sus_queue, tid);
    }
    if(target == NULL) {
      target = find_tid(finish_list, tid);
      if(target != NULL) {
        return SUCCESS;
      }
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
    target->td->wait_tids[target->td->wait_index] = mainthread->tid;

    if(mainnode->td->waiting_index < 0) {
      mainnode->td->waiting_index = 0;
    }  else {
      mainnode->td->waiting_index ++;
    }
    if(mainnode->td->waiting_index >= mainnode->td->waiting_size) {
      mainnode->td->waiting_size += ARRSIZE;
      mainnode->td->waiting = realloc(mainnode->td->waiting, sizeof(int) * mainnode->td->waiting_size);
    }
    mainnode->td->waiting[mainnode->td->waiting_index] = target->td->tid;

    mainnode->td->state = STOPPED;
    mainnode->td->priority = target->td->priority;
    insert_tail(sus_queue, mainnode);
    makecontext(schedule, (void (*)(void))scheduler, 2, schedule_policy, TRUE);
    swapcontext(mainnode->td->uc, schedule);
  } else {
    if(schedule_policy == FIFO || schedule_policy == SJF) {
      tnode* target;
      if(schedule_policy == FIFO) {
        target = find_tid(fifo_queue, tid);
        if(target == NULL) {
          target = find_tid(sus_queue, tid);
        }
      } else if(schedule_policy == SJF) {
        target = find_tid(sjf_queue, tid);
        if(target == NULL) {
          target = find_tid(sus_queue, tid);
        }
      }
      if(target == NULL) {
        target = find_tid(finish_list, tid);
        if(target == NULL) {
          return FAIL;
        }
        return SUCCESS;
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

      if(head->td->waiting_index < 0) {
        head->td->waiting_index = 0;
      }  else {
        head->td->waiting_index ++;
      }
      if(head->td->waiting_index >= head->td->waiting_size) {
        head->td->waiting_size += ARRSIZE;
        head->td->waiting = realloc(head->td->waiting, sizeof(int) * head->td->waiting_size);
      }
      head->td->waiting[head->td->waiting_index] = target->td->tid;

      head->td->state = STOPPED;
      makecontext(schedule, (void (*)(void))scheduler, 2, schedule_policy, TRUE);
      gettimeofday(head->td->start, NULL);
      swapcontext(head->td->uc, schedule);
      return SUCCESS;
    } else if(schedule_policy == PRIORITY) {
      sigprocmask(SIG_BLOCK, &blocked, NULL);
      tnode* target = find_priority(tid);
      if(target == NULL) {
        target = find_tid(sus_queue, tid);
      }
      if(target == NULL) {
        target = find_tid(finish_list, tid);
        if(target == NULL) {
          return FAIL;
        }
        return SUCCESS;
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

      if(head->td->waiting_index < 0) {
        head->td->waiting_index = 0;
      }  else {
        head->td->waiting_index ++;
      }
      if(head->td->waiting_index >= head->td->waiting_size) {
        head->td->waiting_size += ARRSIZE;
        head->td->waiting = realloc(head->td->waiting, sizeof(int) * head->td->waiting_size);
      }
      head->td->waiting[head->td->waiting_index] = target->td->tid;

      head->td->state = STOPPED;
      makecontext(schedule, (void (*)(void))scheduler, 2, PRIORITY, TRUE);
      swapcontext(head->td->uc, schedule);
      return SUCCESS;
    }

  }
  return SUCCESS;
}

int thread_yield(void) {

  if(lib_init != TRUE || lib_term == TRUE) {
    return FAIL;
  }

  if(head == NULL) {
    return FAIL;
  }

  if(schedule_policy == PRIORITY) {
    sigprocmask(SIG_BLOCK, &blocked, NULL);
  }
  head->td->state = STOPPED;
  makecontext(schedule, (void (*) (void))scheduler, 2, schedule_policy, FALSE);
  swapcontext(head->td->uc, schedule);
  return SUCCESS;

}
