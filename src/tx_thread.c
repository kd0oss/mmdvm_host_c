#include "config.h"
#include "modem.h"
#include "mode.h"
#include "repeater_control.h"
#include <time.h>
#include <string.h>
#include <stdatomic.h>
#include "mmdvm_proto.h"
#include "time_util.h"

// NEW: access config flags
extern config_t* g_cfg;

extern repeater_ctrl_t g_rc;
extern mode_handler_t* g_dmr;
extern mode_handler_t* g_ysf;
extern mode_handler_t* g_nxdn;
extern modem_t* g_modem;

// Stop TX when watchdog expires
static inline void tx_tick() {
  uint32_t now32 = (uint32_t)now_ms();
  if (rc_is_dmr_tx_on(&g_rc) && rc_dmr_tx_watchdog_expired(&g_rc, now32)) {
    modem_dmr_start(g_modem, 0);      // turn TX OFF (0x00)
    rc_set_dmr_tx_on(&g_rc, 0U);
  }
}

void* tx_thread_fn(void* arg) {
  (void)arg;
  uint8_t out[256];
  size_t out_len;

  for (;;) {
    uint32_t n = (uint32_t)now_ms();

    // Always check watchdog first
    tx_tick();

    unsigned rx_active_dmr  = atomic_load_explicit(&g_rc.rx_active_dmr,  memory_order_acquire);
    unsigned rx_active_ysf  = atomic_load_explicit(&g_rc.rx_active_ysf,  memory_order_acquire);
    unsigned rx_active_nxdn = atomic_load_explicit(&g_rc.rx_active_nxdn, memory_order_acquire);

    // DMR echo only if allowed and RX active
    if (g_cfg && g_cfg->echo_dmr && rx_active_dmr && tx_permitted(&g_rc, rx_active_dmr)) {
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
