#include "config.h"
#include "mode.h"
#include "repeater_control.h"
#include "modem.h"
#include "log.h"
#include "time_util.h"
#include "mmdvm_proto.h"
#include <time.h>
#include <stdatomic.h>

extern config_t* g_cfg;
extern repeater_ctrl_t g_rc;
extern mode_handler_t* g_dmr;
extern mode_handler_t* g_ysf;
extern mode_handler_t* g_nxdn;
extern modem_t* g_modem;

// External function from mode_dmr.c
extern int dmr_any_active(mode_handler_t* self);

static void tx_tick(void) {
  uint32_t now = (uint32_t)now_ms();
  
  // Call mode tick functions
  if (g_dmr && g_dmr->ops && g_dmr->ops->tick) {
    g_dmr->ops->tick(g_dmr, now);
  }
  if (g_ysf && g_ysf->ops && g_ysf->ops->tick) {
    g_ysf->ops->tick(g_ysf, now);
  }
  if (g_nxdn && g_nxdn->ops && g_nxdn->ops->tick) {
    g_nxdn->ops->tick(g_nxdn, now);
  }
  
  // Check DMR tail expiration and stop TX if needed
  if (g_dmr && rc_is_dmr_tx_on(&g_rc)) {
    if (!dmr_any_active(g_dmr)) {
      log_info("[TX] All DMR slots inactive, stopping TX");
      modem_dmr_start(g_modem, 0);
      rc_set_dmr_tx_on(&g_rc, 0U);
    }
  }
  
  // Check DMR watchdog
  if (rc_is_dmr_tx_on(&g_rc) && rc_dmr_tx_watchdog_expired(&g_rc, now)) {
    log_info("[TX] DMR watchdog expired, stopping TX");
    modem_dmr_start(g_modem, 0);
    rc_set_dmr_tx_on(&g_rc, 0U);
  }
}

void* tx_thread_fn(void* arg) {
  (void)arg;
  uint8_t out[256];
  size_t out_len;

  for (;;) {
    uint32_t n = (uint32_t)now_ms();

    // Always check watchdog first and call mode ticks
    tx_tick();

    unsigned rx_active_dmr  = atomic_load_explicit(&g_rc.rx_active_dmr,  memory_order_acquire);
    unsigned rx_active_ysf  = atomic_load_explicit(&g_rc.rx_active_ysf,  memory_order_acquire);
    unsigned rx_active_nxdn = atomic_load_explicit(&g_rc.rx_active_nxdn, memory_order_acquire);

    // DMR echo only if allowed and RX active OR if in tail mode
    // We need to check dmr_any_active() to allow tail transmission
    int dmr_can_tx = (g_cfg && g_cfg->echo_dmr && 
                      (rx_active_dmr || (g_dmr && dmr_any_active(g_dmr))) &&
                      tx_permitted(&g_rc, rx_active_dmr));
    
    if (dmr_can_tx) {
      out_len = sizeof(out);
      if (mode_build_tx(g_dmr, out, &out_len, n) == 0 && out_len) {
        uint8_t type = (g_dmr->last_type == MMDVM_DMR_DATA2) ? MMDVM_DMR_DATA2 : MMDVM_DMR_DATA1;
        modem_write_envelope(g_modem, type, out, out_len);
        rc_on_tx_dmr(&g_rc, n);
        // Bump watchdog ONLY on actual TX, so TX mode can turn off if no frames are sent
        rc_bump_dmr_tx_watchdog(&g_rc, n, g_dmr ? g_dmr->hang_ms : 3000);
      }
    }

    // YSF echo gated (default disabled)
    if (g_cfg && g_cfg->echo_ysf && rx_active_ysf && tx_permitted(&g_rc, rx_active_ysf)) {
      out_len = sizeof(out);
      if (mode_build_tx(g_ysf, out, &out_len, n) == 0 && out_len) {
        uint8_t type = (g_ysf->last_type ? g_ysf->last_type : MMDVM_YSF_DATA);
        modem_write_envelope(g_modem, type, out, out_len);
        rc_on_tx_ysf(&g_rc, n);
      }
    }

    // NXDN echo gated (default disabled)
    if (g_cfg && g_cfg->echo_nxdn && rx_active_nxdn && tx_permitted(&g_rc, rx_active_nxdn)) {
      out_len = sizeof(out);
      if (mode_build_tx(g_nxdn, out, &out_len, n) == 0 && out_len) {
        uint8_t type = (g_nxdn->last_type ? g_nxdn->last_type : MMDVM_NXDN_DATA);
        modem_write_envelope(g_modem, type, out, out_len);
        rc_on_tx_nxdn(&g_rc, n);
      }
    }

    struct timespec req = {.tv_sec=0, .tv_nsec=2*1000*1000};
    nanosleep(&req, NULL);
  }
  return NULL;
}
