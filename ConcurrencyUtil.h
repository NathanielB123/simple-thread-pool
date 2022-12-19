#ifndef CONCURRENCY_UTIL_H
#define CONCURRENCY_UTIL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

int get_hardware_concurrency(void);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);
bool mutex_held_by_current_thread(pthread_mutex_t *mutex);
void sema_up(sem_t *sema);
void sema_down(sem_t *sema);
bool sema_try_down(sem_t *sema);

void binary_sema_set_zero(sem_t *sema);
void binary_sema_set_one(sem_t *sema);
void binary_sema_wait(sem_t *sema);

#endif