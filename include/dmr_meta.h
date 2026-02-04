#pragma once
#include <stdint.h>
#include "dmr_defines.h"  // use authoritative values

// These values align with MMDVMHost usage for modem->host payload metadata
// TAG_DATA is already defined in dmr_defines.h as 0x00; keep alias if desired.
#ifndef TAG_DATA
#define TAG_DATA        0x00  // First payload byte for "data" frames (MMDVMHost checks equality to TAG_DATA)
#endif

#ifndef DMR_IDLE_RX
#define DMR_IDLE_RX     0x80  // Bit indicating "idle RX" context
#endif

// Do NOT redefine DMR_SYNC_AUDIO or DMR_SYNC_DATA here; use values from dmr_defines.h:
// DMR_SYNC_AUDIO = 0x20
// DMR_SYNC_DATA  = 0x40

#ifndef DT_CSBK
#define DT_CSBK         0x03  // Data Type value for CSBK
#endif
