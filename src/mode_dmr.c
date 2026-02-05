#include "mode.h"
#include "mode_payloads.h"
#include "mmdvm_proto.h"
#include "dmr_defines.h"   // DT_* macros from meta0 lower nibble
#include <string.h>
#include <stdlib.h>
#include "log.h"

typedef struct {
  int tx_pending;
  uint8_t payload[DMR_FRAME_LENGTH_BYTES + 3]; // exactly 34 RF bytes: control + 33
  size_t payload_len;
} dmr_state_t;

static int dmr_init(mode_handler_t* self) {
  dmr_state_t* s = calloc(1, sizeof(dmr_state_t));
  if (!s) return -1;
  self->state = s;
  self->hang_ms = 3000;
  return 0;
}

static void dmr_fini(mode_handler_t* self) {
  free(self->state);
  self->state = NULL;
}

// Echo only when meta0 indicates a payload burst, and always copy 34 RF bytes from meta1.
// Frame layouts from your modem:
// - meta0 (sync+DT) + RF(control+33)                     => total len >= 35
// - meta0 (sync+DT) + RF(control+33) + RSSI (>=1 bytes)  => total len >= 36
// We ignore RSSI and only forward the RF payload.
static int dmr_on_rx(mode_handler_t* self, const uint8_t* buf, size_t len, uint64_t ts_ms)
{
  dmr_state_t* s = (dmr_state_t*)self->state;
  self->last_activity_ms = ts_ms;

//  log_info("========================== Len: %u", len);
  // Need at least meta0 + RF(control+33) => 35 bytes to echo
  if (len < (DMR_FRAME_LENGTH_BYTES + 1))
  {
    s->tx_pending = 0;
    s->payload_len = 0;
    return 0;
  }

  const uint8_t meta0 = buf[0];
  const uint8_t dt    = (uint8_t)(meta0 & 0x0F);
  const uint8_t* rf   = buf; // RF starts at meta1
  const size_t rf_need = DMR_FRAME_LENGTH_BYTES + 1; // 34 bytes: control + 33
//log_info("========================== DT: %02X", dt);
  // Suppress control bursts and non-payloads:
  // - Voice LC Header (0x01)
  // - Terminator with LC (0x02)
  // - CSBK (0x03)
  // - Data Header (0x06)
  // - Idle (0x09)
  // Echo Voice PI Header (0x00) and payload data (e.g., 0x04, 0x05, 0x07, 0x08)
  switch (dt) {
    case DT_VOICE_LC_HEADER:
    case DT_TERMINATOR_WITH_LC:
    case DT_CSBK:
    case DT_DATA_HEADER:
    case DT_IDLE:
      s->tx_pending = 0;
      s->payload_len = 0;
      return 0;
    default:
      break;
  }

  // Ensure we have at least 34 RF bytes (ignore any RSSI beyond that)
  if (len < (rf_need))
  {
    // Not enough RF bytes to echo; drop safely
    s->tx_pending = 0;
    s->payload_len = 0;
    return 0;
  }

  memcpy(s->payload, rf, rf_need);
  s->payload_len = rf_need;
  s->tx_pending = 1;
  return 0;
}

static void dmr_tick(mode_handler_t* self, uint64_t now_ms) {
  (void)self;
  (void)now_ms;
}

static int dmr_build_tx(mode_handler_t* self, uint8_t* out, size_t* out_len, uint64_t now_ms) {
  (void)now_ms;
  dmr_state_t* s = (dmr_state_t*)self->state;
  if (!s->tx_pending || s->payload_len == 0) {
    *out_len = 0;
    return 0;
  }
  size_t n = s->payload_len;
  if (*out_len < n) n = *out_len;
  memcpy(out, s->payload, n);
  *out_len = n;
  s->tx_pending = 0;
  return 0;
}

static const char* dmr_name(const mode_handler_t* self) {
  (void)self;
  return "DMR";
}

static const mode_ops_t DMR_OPS = {
  .init = dmr_init,
  .fini = dmr_fini,
  .on_rx_frame = dmr_on_rx,
  .tick = dmr_tick,
  .build_tx_frame = dmr_build_tx,
  .name = dmr_name,
};

mode_handler_t* dmr_mode_new(void* ctx) {
  mode_handler_t* h = calloc(1, sizeof(*h));
  h->ops = &DMR_OPS;
  h->ctx = ctx;
  return h;
}

// NEW: cancel any pending echo frame (to avoid stray TX after terminator)
void dmr_mode_cancel_tx(mode_handler_t* h) {
  if (!h || !h->state) return;
  dmr_state_t* s = (dmr_state_t*)h->state;
  s->tx_pending = 0;
  s->payload_len = 0;
}
