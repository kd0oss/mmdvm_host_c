#pragma once
#include <stdint.h>

// These values align with MMDVMHost usage for modem->host payload metadata
#define TAG_DATA        0x00  // First payload byte for "data" frames (MMDVMHost checks equality to TAG_DATA)
#define DMR_IDLE_RX     0x80  // Bit indicating "idle RX" context
#define DMR_SYNC_AUDIO  0x10  // Voice sync present
#define DMR_SYNC_DATA   0x20  // Data sync present
#define DT_CSBK         0x03  // Data Type value for CSBK
