#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include "userthread.h"

#define N 6

void foo(void* val) {
  int num = *(int*)val;
  printf("Created and scheduled %d threads\n", num+1);
}

int main(void) {
  printf(" * Simply testing priority create and join (Really Basic Test)! \n");
  printf(" * Order of running depends on implementation.\n");
  
  if (thread_libinit(PRIORITY) == -1)
    exit(EXIT_FAILURE);

  int tids[N];
  memset(tids, -1, sizeof(tids));
  int temp[N];
  for (int i = 0; i < 2; i++)  {
    temp[i] = i;
    tids[i] = thread_create(foo, &temp[i], 0);
  }
   for (int i = 2; i < 4; i++)  {
     temp[i] = i;
     tids[i] = thread_create(foo, &temp[i], -1);
  }
    for (int i = 4; i < 6; i++)  {
      temp[i] = i;
      tids[i] = thread_create(foo, &temp[i], 1);
  }

  for (int i = 0; i < N; i++)  {
    if (tids[i] == -1)
      exit(EXIT_FAILURE);
  }

  for (int i = 0; i < N; i++)  {
    if (thread_join(tids[i]) == -1) {
      exit(EXIT_FAILURE);
    }
  }

  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}
