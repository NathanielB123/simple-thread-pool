#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

struct job_queue {
  struct job *head;
  struct job **tail;
  sem_t jobs_remaining;
  pthread_mutex_t queue_lock;
  pthread_mutex_t synch_lock;
  sem_t idle;
};

struct thread_pool {
  pthread_t *threads;
  size_t num_threads;
  struct job_queue job_queue;
};

void thread_pool_destroy(struct thread_pool *pool);
bool thread_pool_init(struct thread_pool *pool, unsigned *num_threads);
bool thread_pool_add_submit_job(struct thread_pool *pool, void (*func)(void *),
                                void *aux);
void thread_pool_wait(struct thread_pool *pool);