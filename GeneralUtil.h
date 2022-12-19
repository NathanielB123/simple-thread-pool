#ifndef GENERAL_UTIL_H
#define GENERAL_UTIL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PANIC(msg)                       \
  {                                      \
    fprintf(stderr, "Error: %s\n", msg); \
    exit(EXIT_FAILURE);                  \
  }

#define ARR_CNT(arr) (sizeof(arr) / sizeof(arr[0]))

static inline void *panic_if_null(void *ptr) {
  if (ptr == NULL) PANIC("Unexpected null pointer encountered!");
  return ptr;
}

#undef PANIC_IF_NULL

/* Functions used for multi-step initialisations that could fail at any point.
 * Relies on the initialiser and destructor initialising/destroying members in
 * the same order */

static inline bool track_for_destruction(unsigned *progress, bool success) {
  if (!success) return false;
  ++*progress;
  return true;
}

static inline bool remaining_pending_destruction(unsigned *progress) {
  if (progress == NULL) return true;
  if (*progress == 0) return false;
  --*progress;
  return true;
}

static inline int int_min(int a, int b) { return a < b ? a : b; }

static inline int ceil_div(int numerator, int divisor) {
  return numerator / divisor + (numerator % divisor == 0 ? 0 : 1);
}

#endif