#pragma once
#include <stdint.h>

// Envelope type codes (subset used here)
#define MMDVM_SOF           0xE0
#define MMDVM_GET_VERSION   0x00
#define MMDVM_GET_STATUS    0x01
#define MMDVM_SET_CONFIG    0x02
#define MMDVM_SET_MODE      0x03
#define MMDVM_SET_FREQ      0x04
#define MMDVM_ACK           0x70
#define MMDVM_NAK           0x7F

// DMR specific
#define MMDVM_DMR_DATA1     0x18
#define MMDVM_DMR_DATA2     0x1A
#define MMDVM_DMR_SHORTLC   0x1C
#define MMDVM_DMR_START     0x1D
#define MMDVM_DMR_ABORT     0x1E

// MMDVM SET_MODE codes (external protocol values)
#define MMDVM_MODE_IDLE     0x00
#define MMDVM_MODE_DMR      0x02
#define MMDVM_MODE_YSF      0x03
#define MMDVM_MODE_NXDN     0x05
