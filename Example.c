#include <stdio.h>
#include <stdlib.h>

#include "ThreadPool.h"

#define NUM_TASKS 10

void task(void *aux) {
  unsigned task_num = *(unsigned *)aux;
  printf("Start task %u\n", task_num);
}

int main(int argc, char **argv) {
  static unsigned task_data[NUM_TASKS];
  struct thread_pool tp;
  thread_pool_init(&tp, NULL);
  for (unsigned i = 0; i < NUM_TASKS; ++i) {
    task_data[i] = i;
    thread_pool_add_submit_job(&tp, task, &task_data[i]);
  }
  thread_pool_destroy(&tp);
  return EXIT_SUCCESS;
}