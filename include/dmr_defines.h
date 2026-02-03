#pragma once
#include <stdint.h>

// Data Types
#define DT_VOICE_PI_HEADER     0x00
#define DT_VOICE_LC_HEADER     0x01
#define DT_TERMINATOR_WITH_LC  0x02
#define DT_CSBK                0x03
#define DT_DATA_HEADER         0x06
#define DT_RATE_12_DATA        0x07
#define DT_RATE_34_DATA        0x08
#define DT_IDLE                0x09

// Sync/meta flags in modem payload
#define DMR_IDLE_RX            0x80
#define DMR_SYNC_DATA          0x40
#define DMR_SYNC_AUDIO         0x20

// Frame tagging
#define TAG_DATA               0x00

// CRC masks (from MMDVMHost DMRDefines.h)
static const uint8_t CSBK_CRC_MASK[2] = { 0xA5, 0xA5 };
static const uint8_t DATA_HEADER_CRC_MASK[2] = { 0xCC, 0xCC };

// CSBK opcodes
typedef enum {
  CSBKO_NONE       = 0x00,
  CSBKO_UUVREQ     = 0x04,
  CSBKO_UUANSRSP   = 0x05,
  CSBKO_CTCSBK     = 0x07,
  CSBKO_CALL_ALERT = 0x1F,
  CSBKO_CALL_ALERT_ACK = 0x20,
  CSBKO_RADIO_CHECK    = 0x24,
  CSBKO_NACKRSP       = 0x26,
  CSBKO_CALL_EMERGENCY = 0x27,
  CSBKO_BSDWNACT      = 0x38,
  CSBKO_PRECCSBK      = 0x3D
} csbko_t;
