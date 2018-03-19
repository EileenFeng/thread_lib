#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

#define FAIL -1
#define SUCCESS 1

void yield() {
  
  for (int i = 0; i < 5; i++) {
    sleep(1);
    thread_yield();
    printf("current thread yield for %d times\n", i+1);  
  }
}

int main() {

  printf("* Simple test for fifo thread yield itself\n");
  printf("* should print out five times 'current thread yield for <number> times' \n");
  
  if(thread_libinit(FIFO) == FAIL) {
    printf("Initialize library failed. Exiting...\n");
    exit(EXIT_FAILURE);
  }
  
  int tid = thread_create(yield,NULL,  -1);

  thread_join(tid);
  
  printf("back to main context, terminate library\n");
  if(thread_libterminate() == FAIL) {
    printf("terminate library failed\n");
  }
}
