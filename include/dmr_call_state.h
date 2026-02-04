#pragma once
#include <stdint.h>

typedef struct {
  uint8_t  active;       // 1 if a call is in progress
  uint8_t  color_code;   // 0..15 (from CSBK CBF when available, else meta)
  uint8_t  slot;         // 1 or 2
  uint32_t src_id;       // optional, if discovered
  uint32_t dst_id;       // from CSBK when present
} dmr_call_state_t;
