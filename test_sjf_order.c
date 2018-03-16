#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "userthread.h"

void yield(char* number) {
  for(int i = 0; i < 5; i ++) {
    printf("yeilding  %s %d\n", number, i);
    thread_yield();
    sleep(1);
  }
}


void normal(int val) {
  printf("wata --==---========= %d\n", val);   
}

int main() {
  thread_libinit(SJF);
  int tids[10];
  for(int i = 0; i < 10; i++) {
    tids[i] = -1;
  }
  int tid = thread_create(yield, "111", -1);
  for(int i = 0; i < 10; i++) {
    tids[i] = thread_create(normal, i, -1);
  }
  for(int i = 0; i < 10; i++) {
    if(tids[i] < 0) {
      break;
    } else {
      printf("joining %d\n", tids[i]);
      if(thread_join(tids[i]) == -1) {
	printf("join %d failed\n", tids[i]);
	break;
      }
    }
  }

  printf("back to context\n");
  thread_libterminate();
}
