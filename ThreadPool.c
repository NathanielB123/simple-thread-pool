#include "ThreadPool.h"

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>

#include "ConcurrencyUtil.h"
#include "GeneralUtil.h"

struct job {
  struct job *next;
  void (*func)(void *);
  void *aux;
};

static struct job *job_queue_pop(struct job_queue *queue, bool lock) {
  if (lock) mutex_lock(&queue->queue_lock);
  struct job *job_to_remove = queue->head;
  if (job_to_remove != NULL) {
    queue->head = job_to_remove->next;
    if (queue->head == NULL) queue->tail = &queue->head;
  }
  if (lock) mutex_unlock(&queue->queue_lock);
  return job_to_remove;
}

static void *process_jobs(void *aux) {
  struct job_queue *queue = (struct job_queue *)aux;
  while (true) {
    mutex_lock(&queue->synch_lock);
    if (!sema_try_down(&queue->jobs_remaining)) {
      binary_sema_set_one(&queue->idle); /* We are idle. */
      sema_down(&queue->jobs_remaining);
    }
    mutex_unlock(&queue->synch_lock);
    struct job *job = job_queue_pop(queue, true);
    /* If thread has been signalled but there are no jobs, threadpool must be
     * being destroyed. */
    if (job == NULL) {
      return NULL;
    }
    job->func(job->aux);
    free(job);
  }
  return NULL;
}

static void job_queue_partial_destroy(struct job_queue *queue, unsigned *prog) {
  if (!remaining_pending_destruction(prog)) return;
  sem_destroy(&queue->jobs_remaining);

  if (!remaining_pending_destruction(prog)) return;
  pthread_mutex_destroy(&queue->queue_lock);

  if (!remaining_pending_destruction(prog)) return;
  sem_destroy(&queue->idle);

  if (!remaining_pending_destruction(prog)) return;
  pthread_mutex_destroy(&queue->synch_lock);

  struct job *to_remove;
  while ((to_remove = job_queue_pop(queue, false)) != NULL) {
    if (to_remove->aux != NULL) free(to_remove->aux);
    free(to_remove);
  }
}

static void job_queue_destroy(struct job_queue *queue) {
  job_queue_partial_destroy(queue, NULL);
}

static bool job_queue_init(struct job_queue *queue) {
  *queue = (struct job_queue){.head = NULL, .tail = &queue->head};

  unsigned prog = 0;

  if (!track_for_destruction(&prog,
                             sem_init(&queue->jobs_remaining, 0, 0) == 0) ||
      !track_for_destruction(
          &prog, pthread_mutex_init(&queue->queue_lock, NULL) == 0) ||
      !track_for_destruction(&prog, sem_init(&queue->idle, 0, 1) == 0) ||
      !track_for_destruction(
          &prog, pthread_mutex_init(&queue->synch_lock, NULL) == 0)) {
    job_queue_partial_destroy(queue, &prog);
    return false;
  }

  return true;
}

static void thread_pool_partial_destroy(struct thread_pool *pool,
                                        unsigned *prog) {
  if (prog == NULL) {
    for (size_t i = 0; i < pool->num_threads; ++i) {
      sema_up(&pool->job_queue.jobs_remaining);
    }
    for (size_t i = 0; i < pool->num_threads; ++i) {
      pthread_join(pool->threads[i], NULL);
    }
  }

  if (!remaining_pending_destruction(prog)) return;
  job_queue_destroy(&pool->job_queue);

  if (!remaining_pending_destruction(prog)) return;

  free(pool->threads);
}

void thread_pool_destroy(struct thread_pool *pool) {
  panic_if_null(pool);

  thread_pool_wait(pool);
  thread_pool_partial_destroy(pool, NULL);
}

/* num_threads can be used to specify a specific explicitly the number of worker
 * threads. If NULL, the number of available logical processors is used. */
bool thread_pool_init(struct thread_pool *pool, unsigned *num_threads) {
  panic_if_null(pool);

  *pool = (struct thread_pool){.num_threads = num_threads != NULL
                                                  ? *num_threads
                                                  : get_hardware_concurrency()};

  unsigned prog = 0;

  if (!track_for_destruction(&prog, job_queue_init(&pool->job_queue)) ||
      !track_for_destruction(
          &prog, (pool->threads = malloc(sizeof(*pool->threads) *
                                         pool->num_threads)) != NULL))
    goto CLEAN_UP;

  for (size_t i = 0; i < pool->num_threads; ++i) {
    if (!track_for_destruction(
            &prog, pthread_create(&pool->threads[i], NULL, process_jobs,
                                  &pool->job_queue) == 0))
      goto CLEAN_UP;
  }

  return true;

CLEAN_UP:
  thread_pool_partial_destroy(pool, &prog);
  return false;
}

static bool job_queue_add(struct job_queue *queue, void (*func)(void *),
                          void *aux) {
  struct job *new_job = malloc(sizeof(*new_job));
  if (new_job == NULL) return false;
  *new_job = (struct job){.func = func, .aux = aux, .next = NULL};
  mutex_lock(&queue->queue_lock);
  *queue->tail = new_job;
  queue->tail = &new_job->next;
  mutex_unlock(&queue->queue_lock);
  sema_up(&queue->jobs_remaining);

  binary_sema_set_zero(&queue->idle); /* We are no longer idle. */
  return true;
}

bool thread_pool_add_submit_job(struct thread_pool *pool, void (*func)(void *),
                                void *aux) {
  panic_if_null(pool);
  if (func == NULL) return false;

  return job_queue_add(&pool->job_queue, func, aux);
}

void thread_pool_wait(struct thread_pool *pool) {
  binary_sema_wait(&pool->job_queue.idle);
}
