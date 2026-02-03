#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <stdint.h>

uint64_t now_ms32(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t ms = (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
  return (uint32_t)ms;
}
