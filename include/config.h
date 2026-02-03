#pragma once
#include <stdint.h>
#include "duplex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct config_t {
  char* serial_dev;
  int   baud;
  duplex_mode_t duplex;

  int      dmr_color_code;
  int      dmr_allow_slot1;
  int      dmr_allow_slot2;
  uint32_t dmr_hang_ms;

  uint32_t ysf_node_id;
  uint32_t ysf_hang_ms;

  int      nxdn_ran;
  uint32_t nxdn_hang_ms;

  // NEW: frame logging options
  int      log_frames_rx;
  int      log_frames_tx;

  // NEW: per-mode echo controls
  int      echo_dmr;
  int      echo_ysf;
  int      echo_nxdn;
} config_t;

config_t* config_load(const char* path);
void      config_free(config_t* cfg);

#ifdef __cplusplus
}
#endif
