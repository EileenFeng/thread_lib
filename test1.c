#include <stdlib.h>
#include <stdio.h>
#include "userthread.h"


void hello(int val) {
  printf("hahahhaha--==--==--== %d hello\n", val);
}


int main() {
  printf("initialize\n");
  thread_libinit(FIFO);
  printf("finish initializing\n");
  int tid = thread_create(hello, 10, -1);
  thread_join(tid);
  thread_libterminate();
}
