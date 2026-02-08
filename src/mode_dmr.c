#include "mode.h"
#include "mode_payloads.h"
#include "mmdvm_proto.h"
#include "dmr_defines.h"
#include <string.h>
#include <stdlib.h>
#include "log.h"

// DMR SYNC patterns from ETSI TS 102 361-1 Table 9.2
static const uint8_t MS_VOICE_SYNC[7]  = {0x07, 0xF7, 0xD5, 0xDD, 0x57, 0xDF, 0xD0};
static const uint8_t MS_DATA_SYNC[7]   = {0x0D, 0x5D, 0x7F, 0x77, 0xFD, 0x75, 0x70};
static const uint8_t BS_VOICE_SYNC[7]  = {0x07, 0x55, 0xFD, 0x7D, 0xF7, 0x5F, 0x70};
static const uint8_t BS_DATA_SYNC[7]   = {0x0D, 0xFF, 0x57, 0xD7, 0x5D, 0xF5, 0xD0};

// DMR idle frame (from MMDVMHost DMRDefines.h)
// This is a complete 33-byte RF payload with BS data sync pattern
static const uint8_t DMR_IDLE_RF[33] = {
  0x53, 0xC2, 0x5E, 0xAB, 0xA8, 0x67, 0x1D, 0xC7, 0x38, 0x3B, 0xD9,
  0x36, 0x00, 0x0D, 0xFF, 0x57, 0xD7, 0x5D, 0xF5, 0xD0, 0x03, 0xF6,
  0xE4, 0x65, 0x17, 0x1B, 0x48, 0xCA, 0x6D, 0x4F, 0xC6, 0x10, 0xB4
};

#define TAIL_TIME_MS 4000  // 4 seconds of idle frames after termination

typedef struct {
  uint8_t active;           // 1 if slot has active call
  uint8_t in_tail;          // 1 if in tail/hangtime mode
  uint64_t tail_end_time;   // When tail period ends
  uint64_t last_tx_time;    // Last time we transmitted on this slot
  int tx_pending;           // 1 if we have a frame to send
  uint8_t payload[34];      // Frame to transmit
  size_t payload_len;       // Length of frame
} slot_state_t;

typedef struct {
  slot_state_t slot1;
  slot_state_t slot2;
} dmr_state_t;

static int dmr_init(mode_handler_t* self) {
  dmr_state_t* s = calloc(1, sizeof(dmr_state_t));
  if (!s) return -1;
  self->state = s;
  self->hang_ms = 3000;
  memset(&s->slot1, 0, sizeof(slot_state_t));
  memset(&s->slot2, 0, sizeof(slot_state_t));
  return 0;
}

static void dmr_fini(mode_handler_t* self) {
  free(self->state);
  self->state = NULL;
}

// Convert MS-sourced sync to BS-sourced sync for repeater transmission
static void convert_sync_ms_to_bs(uint8_t* payload, size_t len) {
  // DMR frame structure (34 bytes total):
  // [0]: meta0
  // [1]: control byte
  // [2-13]: info bits (12 bytes)
  // [14]: SYNC byte 0 (mixed with AMBE - do NOT change)
  // [15-20]: SYNC bytes 1-6 ← These need conversion
  // [21-33]: info bits (13 bytes)
  
  if (len < 21) return;
  
  // SYNC bytes 1-6 are at positions 15-20 (0-indexed)
  const int sync_offset = 15;
  
  if (len < sync_offset + 6) return;
  
  uint8_t* sync = &payload[sync_offset];
  
  // Check if this matches MS voice sync bytes 1-6
  int is_ms_voice = (sync[0] == MS_VOICE_SYNC[1] && 
                     sync[1] == MS_VOICE_SYNC[2] && 
                     sync[2] == MS_VOICE_SYNC[3] &&
                     sync[3] == MS_VOICE_SYNC[4] &&
                     sync[4] == MS_VOICE_SYNC[5]);
  
  // Check if this matches MS data sync bytes 1-6
  int is_ms_data  = (sync[0] == MS_DATA_SYNC[1] && 
                     sync[1] == MS_DATA_SYNC[2] && 
                     sync[2] == MS_DATA_SYNC[3] &&
                     sync[3] == MS_DATA_SYNC[4] &&
                     sync[4] == MS_DATA_SYNC[5]);
  
  if (is_ms_voice) {
    // Convert MS voice → BS voice (bytes 1-6)
    memcpy(sync, &BS_VOICE_SYNC[1], 6);
  } else if (is_ms_data) {
    // Convert MS data → BS data (bytes 1-6)
    memcpy(sync, &BS_DATA_SYNC[1], 6);
  }
}

// Determine which slot a frame belongs to from last_type
static uint8_t get_slot_from_type(uint8_t last_type) {
  return (last_type == MMDVM_DMR_DATA2 || last_type == MMDVM_DMR_LOST2) ? 2 : 1;
}

static int dmr_on_rx(mode_handler_t* self, const uint8_t* buf, size_t len, uint64_t ts_ms)
{
  dmr_state_t* s = (dmr_state_t*)self->state;
  self->last_activity_ms = ts_ms;

  // Need at least 34 bytes (meta0 + 33 RF bytes)
  if (len < 34)
  {
    return 0;
  }

  const uint8_t meta0 = buf[0];
  const int has_data_sync = (meta0 & DMR_SYNC_DATA) != 0;
  
  // Determine which slot this frame is for
  uint8_t slot = get_slot_from_type(self->last_type);
  slot_state_t* slot_state = (slot == 2) ? &s->slot2 : &s->slot1;
  
  // Mark slot as active and clear tail mode when we receive frames
  slot_state->active = 1;
  slot_state->in_tail = 0;
  
  // ONLY suppress frames with DATA_SYNC that are control frames
  if (has_data_sync) {
    const uint8_t dt = (uint8_t)(meta0 & 0x0F);
    
    // Suppress control bursts (only valid when has_data_sync)
    switch (dt) {
      case DT_VOICE_LC_HEADER:
      case DT_TERMINATOR_WITH_LC:
      case DT_CSBK:
      case DT_DATA_HEADER:
      case DT_IDLE:
        slot_state->tx_pending = 0;
        return 0;
      default:
        break; // Data payload bursts - echo them
    }
  }
  
  // Echo this frame - copy entire frame including meta0
  memcpy(slot_state->payload, buf, 34);
  slot_state->payload_len = 34;
  
  // Convert MS sync to BS sync for repeater operation
  convert_sync_ms_to_bs(slot_state->payload, 34);
  
  slot_state->tx_pending = 1;
  
  return 0;
}

// Called when a slot receives a terminator
void dmr_slot_terminated(mode_handler_t* self, uint8_t slot, uint64_t now_ms) {
  if (!self || !self->state) return;
  
  dmr_state_t* s = (dmr_state_t*)self->state;
  slot_state_t* slot_state = (slot == 2) ? &s->slot2 : &s->slot1;
  
  log_info("[DMR] Slot %u terminated, entering %dms tail", slot, TAIL_TIME_MS);
  
  // Mark slot as inactive but enter tail mode
  slot_state->active = 0;
  slot_state->in_tail = 1;
  slot_state->tail_end_time = now_ms + TAIL_TIME_MS;
  slot_state->tx_pending = 0;  // Clear any pending voice frames
}

// Called when a slot starts a call (from CSBK or LC Header)
void dmr_slot_started(mode_handler_t* self, uint8_t slot) {
  if (!self || !self->state) return;
  
  dmr_state_t* s = (dmr_state_t*)self->state;
  slot_state_t* slot_state = (slot == 2) ? &s->slot2 : &s->slot1;
  
  log_info("[DMR] Slot %u call started", slot);
  
  // Mark slot as active and clear tail mode
  slot_state->active = 1;
  slot_state->in_tail = 0;
}

// Check if a slot needs idle frame transmission
static int slot_needs_idle(slot_state_t* slot, uint64_t now_ms) {
  if (!slot->in_tail) return 0;
  if (now_ms >= slot->tail_end_time) {
    // Tail expired
    slot->in_tail = 0;
    return 0;
  }
  
  // Send idle frame every 60ms
  if (slot->last_tx_time == 0 || (now_ms - slot->last_tx_time) >= 60) {
    return 1;
  }
  
  return 0;
}

static void dmr_tick(mode_handler_t* self, uint64_t now_ms) {
  dmr_state_t* s = (dmr_state_t*)self->state;
  
  int was_active = s->slot1.in_tail || s->slot2.in_tail;
  
  // Check if tails have expired
  if (s->slot1.in_tail && now_ms >= s->slot1.tail_end_time) {
    log_info("[DMR] Slot 1 tail expired");
    s->slot1.in_tail = 0;
  }
  
  if (s->slot2.in_tail && now_ms >= s->slot2.tail_end_time) {
    log_info("[DMR] Slot 2 tail expired");
    s->slot2.in_tail = 0;
  }
  
  // If we had tails running but now all are expired, we need to signal TX stop
  // But we can't call modem functions directly from here
  // The RX thread will check dmr_any_active() and stop TX
}

static int dmr_build_tx(mode_handler_t* self, uint8_t* out, size_t* out_len, uint64_t now_ms) {
  dmr_state_t* s = (dmr_state_t*)self->state;
  
  // Determine which slot to transmit on based on last_type
  uint8_t slot = get_slot_from_type(self->last_type);
  slot_state_t* slot_state = (slot == 2) ? &s->slot2 : &s->slot1;
  
  // If we have a pending real frame, send it
  if (slot_state->tx_pending && slot_state->payload_len > 0) {
    memcpy(out, slot_state->payload, slot_state->payload_len);
    *out_len = slot_state->payload_len;
    slot_state->tx_pending = 0;
    slot_state->last_tx_time = now_ms;
    return 0;
  }
  
  // Check if this slot needs idle frame transmission
  if (slot_needs_idle(slot_state, now_ms)) {
    // Build idle frame: meta0 + RF payload
    out[0] = DMR_SYNC_DATA | DMR_IDLE_RX | DT_IDLE; // 0xC9
    memcpy(&out[1], DMR_IDLE_RF, 33);
    *out_len = 34;
    slot_state->last_tx_time = now_ms;
    log_info("[DMR] Slot %u: TX idle frame (tail)", slot);
    return 0;
  }
  
  *out_len = 0;
  return 0;
}

// Check if ANY slot is active or in tail
int dmr_any_active(mode_handler_t* self) {
  if (!self || !self->state) return 0;
  dmr_state_t* s = (dmr_state_t*)self->state;
  return s->slot1.active || s->slot1.in_tail || s->slot2.active || s->slot2.in_tail;
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

// Cancel any pending echo frame
void dmr_mode_cancel_tx(mode_handler_t* h) {
  if (!h || !h->state) return;
  dmr_state_t* s = (dmr_state_t*)h->state;
  s->slot1.tx_pending = 0;
  s->slot2.tx_pending = 0;
}
