#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define timespec_sub(after, before, result)                         \
  do {                                                              \
    (result)->tv_sec = (after)->tv_sec - (before)->tv_sec;          \
    (result)->tv_nsec = (after)->tv_nsec - (before)->tv_nsec;       \
    if ((result)->tv_nsec < 0) {                                    \
      --(result)->tv_sec;                                           \
      (result)->tv_nsec += 1000000000;                              \
    }                                                               \
  } while (0)
  
void hexdump(const void *data, const int len, FILE *stream);


#endif