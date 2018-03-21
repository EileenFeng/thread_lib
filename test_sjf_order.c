#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

#define FAIL -1
#define SUCCESS 0

#define N 11

void yield(void* input) {
  for(int i = 0; i < 5; i ++) {
    printf("Current thread is %s %d\n", input, i);
    thread_yield();
    sleep(1);
  }
}


void normal() {
  printf("Hello we are testing sjf order\n");
}

int main() {
  printf("* Test for SJF thread_yield\n");
  printf("* Should print out ten lines of 'Hello we are testing sjf order' before FINISH printing 'Current thread is yielding' message\n");

  if(thread_libinit(SJF) == FAIL) {
    exit(EXIT_FAILURE);
  }

  int tids[N];
  for(int i = 0; i < 10; i++) {
    tids[i] = -1;
  }

  tids[0] = thread_create(yield, "yielding", -1);
  for(int i = 1; i < N; i++) {
    tids[i] = thread_create(normal, NULL, -1);
  }

  for(int i = 0; i < N; i++) {
    if(tids[i] < 0) {
      exit(EXIT_FAILURE);
    } else {
      if(thread_join(tids[i]) == FAIL) {
	       exit(EXIT_FAILURE);
      }
    }
  }

  printf("back to context\n");
  thread_libterminate();
}
