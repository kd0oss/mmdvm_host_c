#include "modem.h"
#include "mode.h"
#include <time.h>
#include "repeater_control.h"
#include <stdlib.h>
#include "time_util.h"
#include "dmr_meta.h"
#include "mmdvm_proto.h"  // declares MMDVM_DMR_START
#include <stdio.h>
#include "dmr_defines.h"
#include "dmr_csbk.h"
#include "dmr_csbk_port.h"  // declares csbk_is_bsdwnact_strict
#include "log.h"  // add this
#include "modes.h"

extern repeater_ctrl_t g_rc;
extern mode_handler_t* g_dmr;
extern mode_handler_t* g_ysf;
extern mode_handler_t* g_nxdn;
extern modem_t* g_modem;

//static uint64_t now_ms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL; }
extern int dmr_is_wakeup_idle_csbk(const uint8_t* payload, size_t len);
extern int modem_dmr_start_slot(modem_t* m, uint8_t slot); // if you kept slot-specific variant
extern int modem_write_envelope(modem_t* m, uint8_t type, const uint8_t* payload, size_t len);

// Or expose a direct start boolean like MMDVMHost:

int modem_dmr_start_tx(modem_t* m, int tx_on) {
fprintf(stderr, "Send start...\n");
  uint8_t p[1] = { tx_on ? 0x01 : 0x00 };
  return modem_write_envelope(m, MMDVM_DMR_START, p, 1);
}

void* rx_thread_fn(void* arg) {
  (void)arg;
  modem_frame_t f;
  for (;;) {
    int r = modem_read_frame(g_modem, &f);
    if (r == 0) {
      uint32_t now = now_ms32();
      switch (f.mode) {
        case MODE_DMR:
          if (rc_repeat_allowed_dmr(&g_rc, now)) {
            g_dmr->last_type = f.type; // slot-aware
            // Check idle+data sync+CSBK metadata first (as in MMDVMHost)
            if (f.len >= 34) {
              uint8_t meta0 = f.data[0];
              uint8_t meta1 = f.data[1];
              // Log metadata so we can confirm wake-up pattern
              log_info("[RX] DMR meta tag=0x%02X meta=0x%02X", meta0, meta1);
              const uint8_t want = (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK); // 0xC3
              if (meta0 == (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK)) {
                // Strict CSBK decode: bytes start at f.data+2
                uint32_t srcId = 0;
                int strict_ok = csbk_is_bsdwnact_strict(f.data + 1, f.len - 1, &srcId);
                log_info("CSBK strict (offset+bitorder probe): %s, srcId=%u", strict_ok ? "PASS" : "FAIL", strict_ok ? srcId : 0U);

                if (strict_ok)  {
                  // Ensure modem is in DMR mode before START
                  int sm = modem_set_mode(g_modem, MODE_DMR);
                  if (sm == 0) {
                    // small settle delay
                    struct timespec req = { .tv_sec = 0, .tv_nsec = 10 * 1000 * 1000 }; // 10ms
                    nanosleep(&req, NULL);
                  } else {
                    log_info("set_mode(MODE_DMR) failed, sm=%d", sm);
                  }
                  // Trigger START per MMDVMHost; boolean tx-on
                  modem_dmr_start(g_modem, 1);
                  rc_set_dmr_tx_on(&g_rc, 1U);
                  rc_bump_dmr_tx_watchdog(&g_rc, now_ms32(), 3000); // 3s TX watchdog like MMDVMHost
                } else {
                  // Optional fallback to validate path; comment if you want strict only
                  modem_dmr_start(g_modem, 1);
                  rc_set_dmr_tx_on(&g_rc, 1U);
                  rc_bump_dmr_tx_watchdog(&g_rc, now_ms32(), 3000); // 3s TX watchdog like MMDVMHost
                }
               }
            }
            mode_on_rx(g_dmr,  f.data, f.len, f.ts_ms);
            rc_on_rx_dmr(&g_rc);
            if (rc_is_dmr_tx_on(&g_rc)) {
              rc_bump_dmr_tx_watchdog(&g_rc, now_ms32(), 3000);
            }          }
          break;
        case MODE_YSF:
          if (rc_repeat_allowed_ysf(&g_rc, now)) {
            g_ysf->last_type = f.type;
            mode_on_rx(g_ysf,  f.data, f.len, f.ts_ms);
            rc_on_rx_ysf(&g_rc);
          }
          break;
        case MODE_NXDN:
          if (rc_repeat_allowed_nxdn(&g_rc, now)) {
            g_nxdn->last_type = f.type;
            mode_on_rx(g_nxdn, f.data, f.len, f.ts_ms);
            rc_on_rx_nxdn(&g_rc);
          }
          break;
      }
      free((void*)f.data);
    }
  }
  return NULL;
}

