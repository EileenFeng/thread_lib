#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "userthread.h"

#define N 6

void foo(int input) {
  for(int i = 0; i < 6; i++) {
    printf("The %dth thread in `tids` array alarmed %d times\n", input, i+1);
    poll(NULL, 0, 100);
  }
  printf("Out of loop in foo %d\n", input);
}

void foo2(int val) {
  printf("Hello World\n");
}

int main(void) {
  printf("* Testing timer and switching for priority scheduling\n");
  printf("* Should print out 'The <num>th thread in `tids` array alarmed' for 6 times for the 3rd and 4th thread in the 'tids' array, before printing out the 'Out of loop in foo' message\n");

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

  printf("back to main context\n");
  
  if (thread_libterminate() == -1)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}


