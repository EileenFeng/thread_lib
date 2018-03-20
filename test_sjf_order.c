#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

#define FAIL -1
#define SUCCESS 0

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
  
  int tids[10];
  
  for(int i = 0; i < 10; i++) {
    tids[i] = -1;
  }
  
  int tid = thread_create(yield, "yielding", -1);
  for(int i = 0; i < 10; i++) {
    tids[i] = thread_create(normal, NULL, -1);
  }
  for(int i = 0; i < 10; i++) {
    if(tids[i] < 0) {
      break;
    } else {
      if(thread_join(tids[i]) == -1) {
	printf("join %d failed\n", tids[i]);
	break;
      }
    }
  }

  printf("back to context\n");
  thread_libterminate();
}
