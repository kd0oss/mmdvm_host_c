#include "modem.h"
#include "mode.h"
#include "repeater_control.h"
#include "time_util.h"
#include "dmr_meta.h"
#include "mmdvm_proto.h"
#include "dmr_defines.h"
#include "dmr_ports.h"
#include "dmr_call_state.h"
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

// Separate call states for each slot
static dmr_call_state_t g_call_slot1;
static dmr_call_state_t g_call_slot2;

// External functions from mode_dmr.c
extern void dmr_slot_terminated(mode_handler_t* self, uint8_t slot, uint64_t now_ms);
extern void dmr_slot_started(mode_handler_t* self, uint8_t slot);
extern int dmr_any_active(mode_handler_t* self);

static inline uint8_t dmr_slot_from_type(uint8_t envelope_type) {
  return (envelope_type == MMDVM_DMR_DATA2 || envelope_type == MMDVM_DMR_LOST2) ? 2 : 1;
}

// Get the call state for a specific slot
static inline dmr_call_state_t* get_slot_state(uint8_t slot) {
  return (slot == 2) ? &g_call_slot2 : &g_call_slot1;
}

void* rx_thread_fn(void* arg) {
  (void)arg;
  dmr_call_reset(&g_call_slot1);
  dmr_call_reset(&g_call_slot2);

  modem_frame_t f;
  for (;;) {
    if (modem_read_frame(g_modem, &f) == 0) {
      uint64_t now64 = now_ms();
      switch (f.mode) {
        case MODE_DMR:
          if (rc_repeat_allowed_dmr(&g_rc, now64)) {
            g_dmr->last_type = f.type;

            if (f.len >= 34) {
              const uint8_t meta0 = f.data[0];
              
              // Extract sync flags from upper nibble
              const int has_data_sync  = (meta0 & DMR_SYNC_DATA) != 0;   // 0x40
              const int has_voice_sync = (meta0 & DMR_SYNC_AUDIO) != 0;  // 0x20
              const int is_idle        = (meta0 & DMR_IDLE_RX) != 0;     // 0x80

              // RF payload begins at meta1
              const uint8_t* payload     = f.data + 1;
              const size_t   payload_len = (f.len - 1);
              const uint8_t  slot        = dmr_slot_from_type(f.type);

              // Get the call state for this slot
              dmr_call_state_t* call = get_slot_state(slot);
              uint8_t cc_effective = call->active ? call->color_code : 0xFF;

              // ===================================================================
              // CONTROL FRAMES (DATA_SYNC) - DT field is valid
              // ===================================================================
              if (has_data_sync) {
                // Only for control frames, extract DT from lower nibble
                const uint8_t dt = (uint8_t)(meta0 & 0x0F);
                
                // CSBK handling
                if (dt == DT_CSBK && is_idle) {
                  uint32_t srcIdStrict = 0;
                  int strict_ok = csbk_is_bsdwnact_strict(payload, payload_len, &srcIdStrict);

                  dmr_csbk_info_t csbk = {0};
                  if (csbk_parse_info(payload, payload_len, &csbk) && csbk.valid) {
                    uint8_t cc_from_csbk = (uint8_t)(csbk.cbf & 0x0F);
                    if (cc_from_csbk) {
                      cc_effective = cc_from_csbk;
                      dmr_call_update_cc(call, cc_effective);
                    }

                    // If not active, start a call
                    if (!call->active) {
                      dmr_call_start(call, cc_effective, slot, csbk.srcId, csbk.dstId, 1);
                      
                      // Notify mode_dmr that this slot has started
                      dmr_slot_started(g_dmr, slot);
                      
                      // Start TX if not already on
                      if (!rc_is_dmr_tx_on(&g_rc)) {
                        if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                        modem_get_status(g_modem);
                        modem_dmr_start(g_modem, 1);
                        rc_set_dmr_tx_on(&g_rc, 1U);
                      }
                      rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                    }
                  }

                  log_info("CSBK strict: %s, srcId=%u, slot=%u", 
                           strict_ok ? "PASS" : "FAIL", strict_ok ? srcIdStrict : 0U, slot);
                }

                // Voice LC Header
                if (dt == DT_VOICE_LC_HEADER && payload_len >= 33)
                {
                  uint32_t src_id = 0, dst_id = 0; int is_group = 1;
                  if (dmr_lc_parse_ids_dt(payload, 33, dt, &src_id, &dst_id, &is_group))
                  {
                    if (!call->active)
                    {
                      dmr_call_start(call, cc_effective, slot, src_id, dst_id, (uint8_t)is_group);
                      
                      // Notify mode_dmr that this slot has started
                      dmr_slot_started(g_dmr, slot);
                      
                      // Start TX if not already on
                      if (!rc_is_dmr_tx_on(&g_rc)) {
                        if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                        modem_get_status(g_modem);
                        modem_dmr_start(g_modem, 1);
                        rc_set_dmr_tx_on(&g_rc, 1U);
                      }
                      rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                    } else
                    {
                      call->src_id = src_id;
                      call->dst_id = dst_id;
                      call->is_group = (uint8_t)is_group;
                    }

                    log_info("[RX] DMR LC Header: src=%u dst=%u %s | slot=%u cc=%s", 
                             src_id, dst_id, is_group ? "(group)" : "(private)",
                             slot, (cc_effective == 0xFF ? "?" : ""));
                    if (cc_effective != 0xFF)
                      log_info("           cc=%u", cc_effective);
                  } else
                  {
                    log_info("[RX] DMR LC: decode FAILED | slot=%u", slot);
                  }
                  
                  // Pass to mode handler (will be filtered, not echoed)
                  mode_on_rx(g_dmr, f.data, f.len, f.ts_ms);
                  rc_on_rx_dmr(&g_rc);
                }

                // Data Header
                if (dt == DT_DATA_HEADER && payload_len >= 33)
                {
                  uint32_t src_id = 0, dst_id = 0; int is_group = 1;
                  if (dmr_lc_parse_ids_dt(payload, 33, dt, &src_id, &dst_id, &is_group))
                  {
                    if (!call->active)
                    {
                      dmr_call_start(call, cc_effective, slot, src_id, dst_id, (uint8_t)is_group);
                      
                      // Notify mode_dmr that this slot has started
                      dmr_slot_started(g_dmr, slot);
                      
                      // Start TX if not already on
                      if (!rc_is_dmr_tx_on(&g_rc)) {
                        if (modem_set_mode(g_modem, MODE_DMR) != 0) log_info("SET_MODE DMR failed");
                        modem_get_status(g_modem);
                        modem_dmr_start(g_modem, 1);
                        rc_set_dmr_tx_on(&g_rc, 1U);
                      }
                      rc_bump_dmr_tx_watchdog(&g_rc, (uint32_t)now64, g_dmr ? g_dmr->hang_ms : 3000);
                    } else
                    {
                      call->src_id = src_id;
                      call->dst_id = dst_id;
                      call->is_group = (uint8_t)is_group;
                    }

                    log_info("[RX] DMR Data Header: src=%u dst=%u %s | slot=%u", 
                             src_id, dst_id, is_group ? "(group)" : "(private)", slot);
                  }
                  
                  // Pass to mode handler
                  mode_on_rx(g_dmr, f.data, f.len, f.ts_ms);
                  rc_on_rx_dmr(&g_rc);
                }

                // Terminator with LC
                if (dt == DT_TERMINATOR_WITH_LC)
                {
                  if (call->active)
                  {
                    log_info("[DMR] Terminator: slot=%u dst=%u", slot, call->dst_id);
                    
                    // Stop the call and enter tail mode for this slot
                    dmr_call_stop(call);
                    dmr_slot_terminated(g_dmr, slot, now64);
                    
                    // Don't stop TX here - tail mode will keep it running
                    // TX will be stopped by the tick function when tails expire
                  }
                }
              }
              
              // ===================================================================
              // VOICE FRAMES WITH SYNC (meta0 = 0x20)
              // ===================================================================
              else if (has_voice_sync && !is_idle && !has_data_sync)
              {
                // Voice payload burst with sync pattern
                if (call->active) {
                  mode_on_rx(g_dmr, f.data, f.len, f.ts_ms);
                  rc_on_rx_dmr(&g_rc);
                  log_info("[RX] DMR VOICE+SYNC slot=%u dst_id=%u", slot, call->dst_id);
                }
              }
              
              // ===================================================================
              // VOICE FRAMES WITHOUT SYNC (meta0 = 0x01-0x05, etc.)
              // ===================================================================
              else if (!has_data_sync && !has_voice_sync && !is_idle)
              {
                // Extract what would be the DT if this were a control frame
                const uint8_t dt_value = (uint8_t)(meta0 & 0x0F);
                
                // Voice bursts - echo if we have an active call on this slot
                if (call->active) {
                  mode_on_rx(g_dmr, f.data, f.len, f.ts_ms);
                  rc_on_rx_dmr(&g_rc);
                  log_info("[RX] DMR VOICE burst=0x%02X slot=%u dst_id=%u", 
                           dt_value, slot, call->dst_id);
                }
              }
            }
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
