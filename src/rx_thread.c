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
#include "dmr_csbk_port.h"  // declares csbk_is_bsdwnact_strict
#include "log.h"

extern repeater_ctrl_t g_rc;
extern mode_handler_t* g_dmr;
extern mode_handler_t* g_ysf;
extern mode_handler_t* g_nxdn;
extern modem_t* g_modem;

extern int modem_write_envelope(modem_t* m, uint8_t type, const uint8_t* payload, size_t len);

// Minimal helper: detect DMR voice/data header (heuristic)
static int is_dmr_voice_header(const uint8_t* payload, size_t len) {
  if (!payload || len < 33) return 0;
  const uint8_t ct = payload[0];
  const uint8_t CONTROL_VOICE = 0x20;
  const uint8_t CONTROL_DATA  = 0x40;
  const uint8_t CONTROL_IDLE  = 0x80;
  return ((ct & (CONTROL_VOICE | CONTROL_DATA)) && !(ct & CONTROL_IDLE)) ? 1 : 0;
}

// Build a basic ShortLC payload from an incoming header (placeholder mapping)
static int build_shortlc_from_header(const uint8_t* header33, uint8_t out9[9]) {
  if (!header33) return -1;
  for (int i = 0; i < 9; ++i) out9[i] = header33[2 + i]; // shift past control/type
  return 0;
}

int modem_dmr_start_tx(modem_t* m, int tx_on) {
  uint8_t p[1] = { tx_on ? 0x01 : 0x00 };
  return modem_write_envelope(m, MMDVM_DMR_START, p, 1);
}

void* rx_thread_fn(void* arg) {
  (void)arg;
  modem_frame_t f;
  for (;;) {
    int r = modem_read_frame(g_modem, &f);
    if (r == 0) {
      uint64_t now64 = now_ms();
      switch (f.mode) {
        case MODE_DMR:
          if (rc_repeat_allowed_dmr(&g_rc, now64)) {
            g_dmr->last_type = f.type;
            if (f.len >= 34) {
              uint8_t meta0 = f.data[0];
              uint8_t meta1 = f.data[1];
              log_info("[RX] DMR meta tag=0x%02X meta=0x%02X", meta0, meta1);

              // CSBK wake-up detection (idle + data sync + CSBK)
              if (meta0 == (DMR_IDLE_RX | DMR_SYNC_DATA | DT_CSBK)) {
                uint32_t srcId = 0;
                int strict_ok = csbk_is_bsdwnact_strict(f.data + 1, f.len - 1, &srcId);
                log_info("CSBK strict: %s, srcId=%u", strict_ok ? "PASS" : "FAIL", strict_ok ? srcId : 0U);

                if (strict_ok)  {
                  // Set DMR mode, poll status, start TX, and set initial watchdog
                  if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                  modem_get_status(g_modem);
                  modem_dmr_start(g_modem, 1);
                  rc_set_dmr_tx_on(&g_rc, 1U);
                  rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                }
              }

              // Voice/data header: send ABORT and ShortLC (mirrors MMDVMHost behavior)
              if (is_dmr_voice_header(f.data, f.len)) {
                modem_dmr_abort(g_modem, 1);
                uint8_t slc9[9];
                if (build_shortlc_from_header(f.data, slc9) == 0) {
                  modem_dmr_shortlc(g_modem, slc9);
                }
              }
            }

            // Echo path input processing
            mode_on_rx(g_dmr,  f.data, f.len, f.ts_ms);
            rc_on_rx_dmr(&g_rc);

            // IMPORTANT: do NOT bump TX watchdog here; it should expire after last TX.
            // If we keep bumping here, TX mode never turns off.
          }
          break;

        case MODE_YSF:
          if (rc_repeat_allowed_ysf(&g_rc, now64)) {
            g_ysf->last_type = f.type;
            mode_on_rx(g_ysf,  f.data, f.len, f.ts_ms);
            rc_on_rx_ysf(&g_rc);
          }
          break;

        case MODE_NXDN:
          if (rc_repeat_allowed_nxdn(&g_rc, now64)) {
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
