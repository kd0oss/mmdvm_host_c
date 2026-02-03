#include "dmr_meta.h"
#include <stddef.h>
#include <stdint.h>

// Returns 1 if metadata indicates a CSBK wake-up while idle; 0 otherwise
int dmr_is_wakeup_idle_csbk(const uint8_t* payload, size_t len) {
  if (!payload || len < 34) return 0;
  uint8_t tag = payload[0];
  if (tag != (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK)) return 0;
  return 1;
}
