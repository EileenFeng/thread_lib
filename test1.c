#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

int t2;


void hello(int val) {
  for(int i = 0; i < 5; i++) {
    printf("------- %d %d ------\n", i, val);
  }
}

void haha() {
  
  for (int i = 0; i < 5; i++) {
    printf("in haha %d\n", i);
    sleep(1);
    thread_yield();
  }
}

void hello2(char* input) {
  printf("wata --==---========= %s\n", input);
}

int main() {
  thread_libinit(FIFO);
  printf("before create\n");
  int tid = thread_create(haha,NULL,  -1);
  printf("after create\n");
  t2 = thread_create(hello, 11, -1);
  int tid3 = thread_create(hello, 12, -1);
  thread_join(tid);
  //  thread_yield();
  
  printf("bac ------k\n");
  thread_libterminate();
}
