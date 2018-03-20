#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "userthread.h"

#define N 6

void foo(void* tid) {

  for(int i = 0; i < 6; i++) {
    printf("Thread with index %d yield %d times\n", *(int*)tid, i+1);
    thread_yield();
  }
  printf("Thread with index %d finished yielding\n", *(int*)tid);
  
}

void foo2(void* tid) {
  printf("Thread with index  %d  is running\n", *(int*)tid);
}

int main(void) {
  printf("* Test:  highest priority threads join lower priority threads\n");
  printf("* THe 3rd and 4th thread should yield 6 times\n");
  printf("* The 1st, 2nd, 5th ,and 6th threads in the 'tids' array should run and finish before the 3rd and 4th threads\n");
  
  if (thread_libinit(PRIORITY) == -1){
    exit(EXIT_FAILURE);
  }
  
  int tids[N];
  int args[N];
  memset(tids, -1, sizeof(tids));
  

  for (int i = 0; i < 2; i++)  {
    args[i] = i;
    tids[i] = thread_create(foo2, &args[i], 0);
  }
  
  for (int i = 2; i < 4; i++)  {
    args[i] = i;
    tids[i] = thread_create(foo, &args[i], -1);
  }
   
  for (int i = 4; i < 6; i++)  {
    args[i] = i;
    tids[i] = thread_create(foo2, &args[i], 1);
  }
    
  for (int i = 0; i < N; i++)  {
    if (tids[i] == -1)
      exit(EXIT_FAILURE);
  }

  thread_join(2);
  
  printf("back to main context\n");
  if (thread_libterminate() == -1){
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
