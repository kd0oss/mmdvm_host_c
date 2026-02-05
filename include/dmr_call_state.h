#pragma once
#include <stdint.h>

typedef struct {
  uint8_t  active;       // 1 if a call is in progress
  uint8_t  color_code;   // 0..15 (from CSBK CBF when available)
  uint8_t  slot;         // 1 or 2 (from envelope type)
  uint32_t src_id;       // last decoded source ID
  uint32_t dst_id;       // last decoded destination ID (TG/private)
  uint8_t  is_group;     // 1 group call, 0 private (from LC FLCO)
} dmr_call_state_t;

void dmr_call_reset(dmr_call_state_t* s);
void dmr_call_start(dmr_call_state_t* s, uint8_t cc, uint8_t slot, uint32_t src, uint32_t dst, uint8_t is_group);
void dmr_call_update_cc(dmr_call_state_t* s, uint8_t cc);
void dmr_call_stop(dmr_call_state_t* s);
