#include "dmr_call_state.h"
#include <string.h>

void dmr_call_reset(dmr_call_state_t* s) { if (s) memset(s, 0, sizeof(*s)); }

void dmr_call_start(dmr_call_state_t* s, uint8_t cc, uint8_t slot, uint32_t src, uint32_t dst, uint8_t is_group) {
  if (!s) return;
  s->active = 1;
  s->color_code = cc;
  s->slot = slot;
  s->src_id = src;
  s->dst_id = dst;
  s->is_group = is_group;
}

void dmr_call_update_cc(dmr_call_state_t* s, uint8_t cc) { if (s && s->active) s->color_code = cc; }

void dmr_call_stop(dmr_call_state_t* s) { if (s) s->active = 0; }
