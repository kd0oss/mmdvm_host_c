#include "modem.h"
#include <time.h>

// Poll GET_STATUS periodically to mirror MMDVMHost behavior and keep the modem stateful/updated.
void* status_thread_fn(void* arg) {
  (void)arg;
  for (;;) {
    modem_get_status((modem_t*)arg);
    struct timespec req = {.tv_sec=0, .tv_nsec=200*1000*1000}; // ~200ms
    nanosleep(&req, NULL);
  }
  return NULL;
}
