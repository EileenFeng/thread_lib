#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "userthread.h"

#define N 6

void foo(int tid) {
  thread_yield();
  //thread_join(3);
  for(int i = 0; i < 7; i++) {
    printf("%d  ---+++---+++---+++---+++\n", tid+2);
  }
}

void foo2(int tid) {
  printf("in Fooooo22222ooooooooooooooo %d\n", tid+2);
}

int main(void) {
  printf(" * Running 6 threads! \n");
  printf("* a simple test that highest priority threads join lower priority threads\n");
  if (thread_libinit(PRIORITY) == -1){
    exit(EXIT_FAILURE);
  }
  int tids[N];
  memset(tids, -1, sizeof(tids));
  

  for (int i = 0; i < 2; i++)  {
    tids[i] = thread_create(foo2, i, 0);
  }
  
  for (int i = 2; i < 4; i++)  {
    tids[i] = thread_create(foo, i, -1);
  }
   
  for (int i = 4; i < 6; i++)  {
    tids[i] = thread_create(foo2, i, 1);
  }
    
  for (int i = 0; i < N; i++)  {
    if (tids[i] == -1)
      exit(EXIT_FAILURE);
  }

  thread_join(2);
  
  printf("baaaack\n");
  if (thread_libterminate() == -1){
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
