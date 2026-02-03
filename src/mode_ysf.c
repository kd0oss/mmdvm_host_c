#include "mode.h"
#include "mode_payloads.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
  int tx_pending;
  uint8_t payload[YSF_FRAME_LENGTH_BYTES + 1];
  size_t payload_len;
} ysf_state_t;

static int ysf_init(mode_handler_t* self) {
  ysf_state_t* s = calloc(1, sizeof(ysf_state_t));
  if (!s) return -1;
  s->tx_pending = 0;
  s->payload_len = 0;
  self->state = s;
  self->hang_ms = 2000;
  return 0;
}
static void ysf_fini(mode_handler_t* self) { free(self->state); self->state = NULL; }
static int ysf_on_rx(mode_handler_t* self, const uint8_t* buf, size_t len, uint64_t ts_ms) {
  ysf_state_t* s = (ysf_state_t*)self->state;
  self->last_activity_ms = ts_ms;
  if (len >= (YSF_FRAME_LENGTH_BYTES + 1)) {
    size_t n = YSF_FRAME_LENGTH_BYTES + 1;
    memcpy(s->payload, buf, n);
    s->payload_len = n;
    s->tx_pending = 1;
  }
  return 0;
}
static void ysf_tick(mode_handler_t* self, uint64_t now_ms) { (void)self; (void)now_ms; }
static int ysf_build_tx(mode_handler_t* self, uint8_t* out, size_t* out_len, uint64_t now_ms) {
  (void)now_ms; ysf_state_t* s = (ysf_state_t*)self->state;
  if (!s->tx_pending || s->payload_len == 0) { *out_len = 0; return 0; }
  size_t n = s->payload_len;
  if (*out_len < n) n = *out_len;
  memcpy(out, s->payload, n);
  *out_len = n;
  s->tx_pending = 0;
  return 0;
}
static const char* ysf_name(const mode_handler_t* self) { (void)self; return "YSF"; }

static const mode_ops_t YSF_OPS = {
  .init = ysf_init, .fini = ysf_fini, .on_rx_frame = ysf_on_rx,
  .tick = ysf_tick, .build_tx_frame = ysf_build_tx, .name = ysf_name,
};

mode_handler_t* ysf_mode_new(void* ctx) { mode_handler_t* h = calloc(1, sizeof(*h)); h->ops = &YSF_OPS; h->ctx = ctx; return h; }
