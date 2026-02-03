#include "repeater_control.h"
#include "mode.h"
#include <time.h>
#include "time_util.h"

// record RX activity
void rc_on_rx_dmr(repeater_ctrl_t* rc) { atomic_store_explicit(&rc->rx_active_dmr, 1U, memory_order_release); }
void rc_on_rx_ysf(repeater_ctrl_t* rc) { atomic_store_explicit(&rc->rx_active_ysf, 1U, memory_order_release); }
void rc_on_rx_nxdn(repeater_ctrl_t* rc){ atomic_store_explicit(&rc->rx_active_nxdn,1U, memory_order_release); }

// record TX events
void rc_on_tx_dmr(repeater_ctrl_t* rc, uint64_t now) {
  atomic_store_explicit(&rc->last_tx_ms_dmr, (uint32_t)now, memory_order_release);
}
void rc_on_tx_ysf(repeater_ctrl_t* rc, uint64_t now) {
  atomic_store_explicit(&rc->last_tx_ms_ysf, (uint32_t)now, memory_order_release);
}
void rc_on_tx_nxdn(repeater_ctrl_t* rc, uint64_t now){
  atomic_store_explicit(&rc->last_tx_ms_nxdn,(uint32_t)now, memory_order_release);
}

static inline int lockout_passed(uint32_t last32, uint32_t now32, uint32_t window_ms) {
  // Unsigned wrap-safe subtraction
  uint32_t diff = now32 - last32;
  return (last32 == 0U) || (diff > window_ms);
}

int rc_repeat_allowed_dmr(const repeater_ctrl_t* rc, uint64_t now) {
  uint32_t last = atomic_load_explicit(&rc->last_tx_ms_dmr, memory_order_acquire);
  return lockout_passed(last, (uint32_t)now, rc->tx_lockout_ms);
}
int rc_repeat_allowed_ysf(const repeater_ctrl_t* rc, uint64_t now) {
  uint32_t last = atomic_load_explicit(&rc->last_tx_ms_ysf, memory_order_acquire);
  return lockout_passed(last, (uint32_t)now, rc->tx_lockout_ms);
}
int rc_repeat_allowed_nxdn(const repeater_ctrl_t* rc, uint64_t now) {
  uint32_t last = atomic_load_explicit(&rc->last_tx_ms_nxdn, memory_order_acquire);
  return lockout_passed(last, (uint32_t)now, rc->tx_lockout_ms);
}

void rc_tick(repeater_ctrl_t* rc, mode_handler_t* dmr, mode_handler_t* ysf, mode_handler_t* nxdn) {
  uint32_t n = (uint32_t)now_ms();
  if (dmr && (n - dmr->last_activity_ms) > 100) atomic_store_explicit(&rc->rx_active_dmr, 0U, memory_order_release);
  if (ysf && (n - ysf->last_activity_ms) > 100) atomic_store_explicit(&rc->rx_active_ysf, 0U, memory_order_release);
  if (nxdn && (n - nxdn->last_activity_ms) > 100) atomic_store_explicit(&rc->rx_active_nxdn,0U, memory_order_release);

  if (dmr) mode_tick(dmr, n);
  if (ysf) mode_tick(ysf, n);
  if (nxdn) mode_tick(nxdn, n);
}
