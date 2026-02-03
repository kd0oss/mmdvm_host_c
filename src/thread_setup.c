#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <string.h>

void set_rt(pthread_t thr, int prio, int cpu) {
  struct sched_param sp = { .sched_priority = prio };
  pthread_setschedparam(thr, SCHED_FIFO, &sp);

  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  pthread_setaffinity_np(thr, sizeof(set), &set);
}
