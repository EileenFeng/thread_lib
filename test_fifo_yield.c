#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

#define FAIL -1
#define SUCCESS 1

#define N 3

void hello() {
  printf("------- Hello World ------\n");
}

void yield() {
  
  thread_yield();
  printf("current thread yielded \n");  
}

int main() {
  int tids[N];

  printf("* Simple test for fifo thread_yield\n");
  printf("* Should print out two lines of '------- Hello World ------' before prints out 'current thread yielded'\n");
  
  if(thread_libinit(FIFO) == FAIL) {
    printf("Initialize library failed. Exiting...\n");
    exit(EXIT_FAILURE);
  }
  
  tids[0] = thread_create(yield,NULL,  -1);
  tids[1] = thread_create(hello, NULL, -1);
  tids[2]  = thread_create(hello, NULL, -1);

  thread_join(tids[0]);
  
  printf("back to main context, terminate library\n");
  if(thread_libterminate() == FAIL) {
    printf("terminate library failed\n");
  }
}
