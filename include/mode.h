#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct mode_handler mode_handler_t;

typedef struct {
  int  (*init)(mode_handler_t* self);
  void (*fini)(mode_handler_t* self);
  int  (*on_rx_frame)(mode_handler_t* self, const uint8_t* buf, size_t len, uint64_t ts_ms);
  void (*tick)(mode_handler_t* self, uint64_t now_ms);
  int  (*build_tx_frame)(mode_handler_t* self, uint8_t* out, size_t* out_len, uint64_t now_ms);
  const char* (*name)(const mode_handler_t* self);
} mode_ops_t;

struct mode_handler {
  const mode_ops_t* ops;
  uint32_t hang_ms;
  uint64_t last_activity_ms;
  void* state;
  void* ctx;
  uint8_t last_type; // NEW: last received envelope TYPE (for DMR slot routing and mode echo)
};

static inline int mode_on_rx(mode_handler_t* h, const uint8_t* b, size_t n, uint64_t ts_ms) { return h->ops->on_rx_frame(h, b, n, ts_ms); }
static inline void mode_tick(mode_handler_t* h, uint64_t now_ms) { h->ops->tick(h, now_ms); }
static inline int mode_build_tx(mode_handler_t* h, uint8_t* out, size_t* out_len, uint64_t now_ms) { return h->ops->build_tx_frame(h, out, out_len, now_ms); }
