#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include "userthread.h"

#define N 128

int count = 0; 
 
void foo() {
  //  printf("haha %d\n", count);
  poll(NULL, 0, 1);
  count ++;
}

int main(void) {
  printf(" * Running 129 threads! \n");

  if (thread_libinit(FIFO) == -1)
    exit(EXIT_FAILURE);

  int tids[N];
  memset(tids, -1, sizeof(tids));

  for (int i = 0; i < N; i++)  {
    tids[i] = thread_create(foo, NULL, 1);
  }

  for (int i = 0; i < N; i++)  {
    if (thread_join(tids[i]) == -1) {
      printf("watatata----\n");
      thread_libterminate();
      exit(EXIT_FAILURE);
    }
  }
  printf("backhahaha0------\n");
  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}
