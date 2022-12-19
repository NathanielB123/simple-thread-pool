#include "ConcurrencyUtil.h"

#include <errno.h>
#include <sys/sysinfo.h>

#include "GeneralUtil.h"

/* Returns the number of logical processors available. */
int get_hardware_concurrency(void) { return get_nprocs(); }

/* Wrappers around synchronisation primitive operations that check for potential
 * errors. */

void mutex_lock(pthread_mutex_t *mutex) {
  int x = pthread_mutex_lock(mutex);
  switch (x) {
    case 0:
      return;
    case EDEADLK:
      PANIC("Deadlock detected!");
    default:
      printf("ERRNO: %i\n", x);
      PANIC("Unexpected error when locking mutex!");
  }
}

bool mutex_held_by_current_thread(pthread_mutex_t *mutex) {
  switch (pthread_mutex_trylock(mutex)) {
    case 0:
      mutex_unlock(mutex);
      return false;
    case EDEADLK:
      return true;
    default:
      PANIC("Unexpected error when trying to lock mutex!");
  }
}

void mutex_unlock(pthread_mutex_t *mutex) {
  switch (pthread_mutex_unlock(mutex)) {
    case 0:
      return;
    case EPERM:
      PANIC("Attempted to unlock mutex that is not owned!");
    default:
      PANIC("Unexpected error when unlocking mutex!");
  }
}

void sema_up(sem_t *sema) {
  if (sem_post(sema) == 0) return;
  switch (errno) {
    case EOVERFLOW:
      PANIC("Semaphore overflow!");
    default:
      PANIC("Unexpected error when upping semaphore!")
  }
}

void sema_down(sem_t *sema) {
  if (sem_wait(sema) == 0) return;
  switch (errno) {
    case EDEADLK:
      PANIC("Deadlock detected!");
    default:
      PANIC("Unexpected error when downing semaphore!")
  }
}

bool sema_try_down(sem_t *sema) {
  if (sem_trywait(sema) == 0) return true;
  switch (errno) {
    case EAGAIN:
      return false;
    case EDEADLK:
      PANIC("Deadlock detected!");
    default:
      PANIC("Unexpected error when trying to down semaphore!");
  }
}

/* Functions for interacting with a binary semaphore (value zero or one). */
/* Useful for signalling of conditions across threads. */

void binary_sema_set_zero(sem_t *sema) { sema_try_down(sema); }

void binary_sema_set_one(sem_t *sema) {
  sema_try_down(sema);
  sema_up(sema);
}

void binary_sema_wait(sem_t *sema) {
  sema_down(sema);
  /* Could probably be more efficient here, but this ensures semaphore value
   * does not exceed 1 even if there is an interleaving. */
  binary_sema_set_one(sema);
}
