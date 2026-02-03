#include "mode.h"
#include "mode_payloads.h"
#include "mmdvm_proto.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
  int tx_pending;
  uint8_t payload[DMR_FRAME_LENGTH_BYTES + 1]; // control + 33
  size_t payload_len;
} dmr_state_t;

static int dmr_init(mode_handler_t* self) {
  dmr_state_t* s = calloc(1, sizeof(dmr_state_t));
  if (!s) return -1;
  self->state = s; self->hang_ms = 3000; return 0;
}

static void dmr_fini(mode_handler_t* self) { free(self->state); self->state = NULL; }

static int dmr_on_rx(mode_handler_t* self, const uint8_t* buf, size_t len, uint64_t ts_ms) {
  dmr_state_t* s = (dmr_state_t*)self->state;
  self->last_activity_ms = ts_ms;

  // Only echo valid DMR VOICE/DATA frames; drop IDLE/other control frames.
  if (len >= (DMR_FRAME_LENGTH_BYTES + 1)) {
    uint8_t ct = buf[0]; // control/type
    const uint8_t CONTROL_VOICE = 0x20;
    const uint8_t CONTROL_DATA  = 0x40;
    const uint8_t CONTROL_IDLE  = 0x80;

    // Must be voice or data, and NOT idle
    if ((ct & (CONTROL_VOICE | CONTROL_DATA)) != 0 && (ct & CONTROL_IDLE) == 0) {
      size_t n = DMR_FRAME_LENGTH_BYTES + 1;
      memcpy(s->payload, buf, n);
      s->payload_len = n;
      s->tx_pending = 1;
    } else {
      // Ignore IDLE or non-voice/data frames to prevent constant echo
      s->tx_pending = 0;
      s->payload_len = 0;
    }
  }
  return 0;
}

static void dmr_tick(mode_handler_t* self, uint64_t now_ms) { (void)self; (void)now_ms; }

static int dmr_build_tx(mode_handler_t* self, uint8_t* out, size_t* out_len, uint64_t now_ms) {
  (void)now_ms; dmr_state_t* s = (dmr_state_t*)self->state;
  if (!s->tx_pending || s->payload_len == 0) { *out_len = 0; return 0; }
  size_t n = s->payload_len;
  if (*out_len < n) n = *out_len;
  memcpy(out, s->payload, n);
  *out_len = n;
  s->tx_pending = 0;
  return 0;
}

static const char* dmr_name(const mode_handler_t* self) { (void)self; return "DMR"; }

static const mode_ops_t DMR_OPS = {
  .init = dmr_init, .fini = dmr_fini, .on_rx_frame = dmr_on_rx,
  .tick = dmr_tick, .build_tx_frame = dmr_build_tx, .name = dmr_name,
};

mode_handler_t* dmr_mode_new(void* ctx) { mode_handler_t* h = calloc(1, sizeof(*h)); h->ops = &DMR_OPS; h->ctx = ctx; return h; }
