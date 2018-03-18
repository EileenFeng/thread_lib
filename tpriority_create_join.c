#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include "userthread.h"

#define N 6

void foo() {
  poll(NULL, 0, 1);
}

int main(void) {
  printf(" * Running 5 threads! \n");

  if (thread_libinit(PRIORITY) == -1)
    exit(EXIT_FAILURE);

  int tids[N];
  memset(tids, -1, sizeof(tids));

  for (int i = 0; i < 2; i++)  {
    tids[i] = thread_create(foo, NULL, 0);
  }
   for (int i = 2; i < 4; i++)  {
    tids[i] = thread_create(foo, NULL, -1);
  }
    for (int i = 4; i < 6; i++)  {
    tids[i] = thread_create(foo, NULL, 1);
  }

  for (int i = 0; i < N; i++)  {
    if (tids[i] == -1)
      exit(EXIT_FAILURE);
  }

  for (int i = 0; i < N; i++)  {
    if (thread_join(tids[i]) == -1)
      exit(EXIT_FAILURE);
  }

  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}
