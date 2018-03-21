#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "userthread.h"

#define N 6

void foo(void* tid) {
  thread_join(*(int*)tid);
  for(int i = 0; i < 7; i++) {
    printf("Joining thread %d\n", *(int*)tid);
  }
}

void foo2(void* val) {
  int temp = *(int*)val;
  printf("Hello Work from thread %d\n", temp+1);
}

int main(void) {
  printf("* Test for joining threads in priority\n");

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
    tids[i] = thread_create(foo, &tids[0], -1);
  }

  for (int i = 4; i < N; i++)  {
    args[i] = i;
    tids[i] = thread_create(foo2, &args[i], 1);
  }

  printf("* Threads %d and thread  %d should not finish until the thread %d finishes\n", tids[2], tids[3], tids[0]);

  for (int i = 0; i < N; i++)  {
    if (tids[i] == -1)
      exit(EXIT_FAILURE);
  }

  thread_join(tids[0]);

  printf("back to main context\n");
  if (thread_libterminate() == -1){
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
