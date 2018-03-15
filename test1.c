#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"


void hello(int val) {
  for(int i = 0; i < 5; i++) {
    //    printf("------- %d %d ------\n", i, val);
    sleep(1);
  }
}

void hello2(char* input) {
  printf("wata --==---== %s\n", input);
}

int main() {
  thread_libinit(FIFO);
  int tid = thread_create(hello, 10, -1);
  int tid2 = thread_create(hello2, "nihao", -1);
  thread_join(tid);
  thread_yield();
  

  thread_libterminate();
}
