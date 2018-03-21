#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include "userthread.h"

#define N 100

void foo(void* arg) {
  int num = *(int*) arg;
  printf("Run %s!\n", arg);
  poll(NULL, 0, 1);
}

int main(void) {
  printf(" * Test for creating and running 100 threads (FIFO) \n");
  printf(" * Once joined the very first thread, the scheduler will automatically start scheduling and running threads from the HEAD of the read queue\n");

  if (thread_libinit(FIFO) == -1)
    exit(EXIT_FAILURE);

  int tids[N];
  memset(tids, -1, sizeof(tids));
  for (int i = 0; i < N; i++)  {
    tids[i] = thread_create(foo, "FIFO", 1);
  }
  for(int i = 0; i < N; i++){
      if(thread_join(tids[i]) == -1){
        exit(EXIT_FAILURE);
      }
  }

  printf("back to main context in test_fifo_create\n");
  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}
