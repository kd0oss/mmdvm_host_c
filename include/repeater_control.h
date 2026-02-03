#pragma once
#include <stdint.h>
#include <stdatomic.h>
#include "duplex.h"

// Forward declare mode_handler_t to avoid including mode.h here
typedef struct mode_handler mode_handler_t;

typedef struct {
  duplex_mode_t mode;
  atomic_uint rx_active_dmr;
  atomic_uint rx_active_ysf;
  atomic_uint rx_active_nxdn;

  // NEW: last TX timestamps per mode, in milliseconds
  atomic_uint last_tx_ms_dmr;
  atomic_uint last_tx_ms_ysf;
  atomic_uint last_tx_ms_nxdn;

  // NEW: track whether DMR TX is currently on (after START, until abort/idle)
  atomic_uint dmr_tx_on;

  atomic_uint dmr_tx_watchdog_ms;

  // NEW: lockout window in ms after TX during which RX echoes are suppressed
  uint32_t tx_lockout_ms;
} repeater_ctrl_t;

static inline int tx_permitted(const repeater_ctrl_t* rc, int is_mode_rx_active) {
  if (rc->mode == DUPLEX_FULL) return 1;
  return is_mode_rx_active ? 0 : 1;
}

// NEW: Prototypes so callers see these functions
void rc_on_rx_dmr(repeater_ctrl_t* rc);
void rc_on_rx_ysf(repeater_ctrl_t* rc);
void rc_on_rx_nxdn(repeater_ctrl_t* rc);

// NEW: Prototype for rc_tick used in main.c
void rc_tick(repeater_ctrl_t* rc, mode_handler_t* dmr, mode_handler_t* ysf, mode_handler_t* nxdn);

// NEW: record TX events
void rc_on_tx_dmr(repeater_ctrl_t* rc, uint64_t now_ms);
void rc_on_tx_ysf(repeater_ctrl_t* rc, uint64_t now_ms);
void rc_on_tx_nxdn(repeater_ctrl_t* rc, uint64_t now_ms);

// NEW: check if repeating RX is allowed (not within lockout)
int rc_repeat_allowed_dmr(const repeater_ctrl_t* rc, uint64_t now_ms);
int rc_repeat_allowed_ysf(const repeater_ctrl_t* rc, uint64_t now_ms);
int rc_repeat_allowed_nxdn(const repeater_ctrl_t* rc, uint64_t now_ms);

// NEW: inline helpers for DMR TX state
static inline void rc_set_dmr_tx_on(repeater_ctrl_t* rc, unsigned on) {
  atomic_store_explicit(&rc->dmr_tx_on, on ? 1U : 0U, memory_order_release);
}
static inline unsigned rc_is_dmr_tx_on(const repeater_ctrl_t* rc) {
  return atomic_load_explicit(&rc->dmr_tx_on, memory_order_acquire);
}

// Wrap-safe bump and expiry for 32-bit ms
static inline void rc_bump_dmr_tx_watchdog(repeater_ctrl_t* rc, uint32_t now_ms, uint32_t hang_ms) {
  atomic_store_explicit(&rc->dmr_tx_watchdog_ms, now_ms + hang_ms, memory_order_release);
}
static inline int rc_dmr_tx_watchdog_expired(const repeater_ctrl_t* rc, uint32_t now_ms) {
  uint32_t until = atomic_load_explicit(&rc->dmr_tx_watchdog_ms, memory_order_acquire);
  return (int32_t)(until - now_ms) <= 0;
}
