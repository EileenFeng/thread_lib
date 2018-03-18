#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "userthread.h"

#define N 6

void foo(int val) {
  poll(NULL, 0, 500);
  
  printf("looooooooooooool %d\n", val);
}

void foo2(int val) {
  //poll(NULL, 0, 1);
  printf("foo2 %d\n", val);
}

int main(void) {
  printf(" * Running 5 threads! \n");

  if (thread_libinit(PRIORITY) == -1)
    exit(EXIT_FAILURE);

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

  for (int i = 0; i < N; i++)  {
    if (thread_join(tids[i]) == -1)
      exit(EXIT_FAILURE);
  }

  printf("baaaack\n");
  
  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}


