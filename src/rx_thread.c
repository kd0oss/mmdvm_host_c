#include "modem.h"
#include "mode.h"
#include "repeater_control.h"
#include "time_util.h"
#include "dmr_meta.h"
#include "mmdvm_proto.h"
#include "dmr_defines.h"
#include "dmr_csbk_port.h"
#include "dmr_lc_port.h"      // LC decode for src/dst/group
#include "dmr_call_state.h"   // NEW: call state
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

extern repeater_ctrl_t g_rc;
extern mode_handler_t* g_dmr;
extern mode_handler_t* g_ysf;
extern mode_handler_t* g_nxdn;
extern modem_t* g_modem;

static dmr_call_state_t g_call;

static inline uint8_t dmr_slot_from_type(uint8_t envelope_type) {
  return (envelope_type == MMDVM_DMR_DATA2 || envelope_type == MMDVM_DMR_LOST2) ? 2 : 1;
}

// LC header detection not gated by VOICE/DATA bits; pass exactly 33 bytes starting at meta1
static int is_dmr_voice_or_data(const uint8_t* header33, size_t len) {
  if (!header33 || len < 33) return 0;
  const uint8_t ct = header33[0];
  const uint8_t CONTROL_VOICE = 0x20;
  const uint8_t CONTROL_DATA  = 0x40;
  const uint8_t CONTROL_IDLE  = 0x80;
  return ((ct & (CONTROL_VOICE | CONTROL_DATA)) && !(ct & CONTROL_IDLE)) ? 1 : 0;
}

static int build_shortlc_from_header_naive(const uint8_t* header33, size_t len, uint8_t out9[9]) {
  if (!header33 || len < 33 || !out9) return 0;
  for (int i = 0; i < 9; ++i) out9[i] = header33[2 + i];
  return 1;
}

void* rx_thread_fn(void* arg) {
  (void)arg;
  dmr_call_reset(&g_call);

  modem_frame_t f;
  for (;;) {
    if (modem_read_frame(g_modem, &f) == 0) {
      uint64_t now64 = now_ms();
      switch (f.mode) {
        case MODE_DMR:
          if (rc_repeat_allowed_dmr(&g_rc, now64)) {
       //     log_info("========================== Meta: %02X", f.data[0]);
            g_dmr->last_type = f.type;

            if (f.len >= 34) {
              const uint8_t meta0 = f.data[0];
              const uint8_t dt    = (uint8_t)(meta0 & 0x0F);          // lower nibble is data type
              const int has_data_sync = (meta0 & DMR_SYNC_DATA) != 0;  // upper nibble sync flag(s)
              const int is_idle       = (meta0 & DMR_IDLE_RX) != 0;

              // RF payload begins at meta1 now
              const uint8_t* payload     = f.data + 1;
              const size_t   payload_len = (f.len - 1);
              const uint8_t  slot        = dmr_slot_from_type(f.type);

              uint8_t  cc_effective = g_call.active ? g_call.color_code : 0xFF;

              // CSBK handling: decode CBF/dstId from coded CSBK in payload
              if (dt == DT_CSBK && has_data_sync && is_idle) {
                uint32_t srcIdStrict = 0;
                int strict_ok = csbk_is_bsdwnact_strict(payload, payload_len, &srcIdStrict);

                dmr_csbk_info_t csbk = {0};
                if (csbk_parse_info(payload, payload_len, &csbk) && csbk.valid) {
                  uint8_t cc_from_csbk = (uint8_t)(csbk.cbf & 0x0F);
                  if (cc_from_csbk) {
                    cc_effective = cc_from_csbk;
                    dmr_call_update_cc(&g_call, cc_effective);
                  }

                  // If not active, start a call with CC and IDs from CSBK when present
                  if (!g_call.active) {
                    dmr_call_start(&g_call, cc_effective, slot, csbk.srcId, csbk.dstId, 1 /* assume group for CSBK */);
                    if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                    modem_get_status(g_modem);
                    modem_dmr_start(g_modem, 1);
                    rc_set_dmr_tx_on(&g_rc, 1U);
                    rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                  }
                }

                log_info("CSBK strict: %s, srcId=%u", strict_ok ? "PASS" : "FAIL", strict_ok ? srcIdStrict : 0U);
              }

              // LC header decode: pass EXACTLY 33 bytes at meta1
              if ((dt == DT_VOICE_LC_HEADER || dt == DT_DATA_HEADER) && payload_len >= 33) {
                uint32_t src_id = 0, dst_id = 0; int is_group = 1;
                if (dmr_lc_parse_ids_dt(payload, 33, dt, &src_id, &dst_id, &is_group)) {
                  // Start call if not active; otherwise update IDs
                  if (!g_call.active) {
                    dmr_call_start(&g_call, cc_effective, slot, src_id, dst_id, (uint8_t)is_group);
                    if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                    modem_get_status(g_modem);
                    modem_dmr_start(g_modem, 1);
                    rc_set_dmr_tx_on(&g_rc, 1U);
                    rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                  } else {
                    g_call.src_id = src_id;
                    g_call.dst_id = dst_id;
                    g_call.is_group = (uint8_t)is_group;
                  }

                  log_info("[RX] DMR LC: src=%u dst=%u %s | slot=%u cc=%s", src_id, dst_id, is_group ? "(group)" : "(private)",
                           slot, (cc_effective == 0xFF ? "?" : ""));
                  if (cc_effective != 0xFF)
                    log_info("           cc=%u", cc_effective);

                  // ABORT then ShortLC (placeholder mapping)
                  //modem_dmr_abort(g_modem, 1);
                  //uint8_t slc9[9];
                  //if (build_shortlc_from_header_naive(payload, 33, slc9)) {
                    //modem_dmr_shortlc(g_modem, slc9);
                  //}
                } else {
                  log_info("[RX] DMR LC: decode FAILED | slot=%u", slot);
                }
              }

              // Stop TX cleanly on Terminator with LC (spec end-of-call)
              if (dt == DT_TERMINATOR_WITH_LC && (meta0 & 0xf0) == 0x40) {
                if (g_call.active) {
                  log_info("[DMR] Terminator with LC: stopping TX (cc=%s slot=%u dst=%u)",
                           (g_call.color_code == 0xFF ? "?" : ""), g_call.slot, g_call.dst_id);
                }
                modem_dmr_start(g_modem, 0);
                rc_set_dmr_tx_on(&g_rc, 0U);
                dmr_call_stop(&g_call);
              }

              // Regular bursts: log dt/slot and any known IDs
              if (dt != DT_VOICE_LC_HEADER && dt != DT_DATA_HEADER /*&& dt != DT_TERMINATOR_WITH_LC*/) {
                if (g_call.dst_id)
                  log_info("[RX] DMR dt=0x%02X slot=%u dst_id=%u", dt, slot, g_call.dst_id);
                else
                  log_info("[RX] DMR dt=0x%02X slot=%u", dt, slot);
              }
            }

            mode_on_rx(g_dmr,  f.data, f.len, f.ts_ms);
            rc_on_rx_dmr(&g_rc);
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
