#pragma once
#include <stdint.h>

// Envelope SOF
#define MMDVM_SOF           0xE0

// Generic commands
#define MMDVM_GET_VERSION   0x00
#define MMDVM_GET_STATUS    0x01
#define MMDVM_SET_CONFIG    0x02
#define MMDVM_SET_MODE      0x03
#define MMDVM_SET_FREQ      0x04

// Acknowledgements
#define MMDVM_ACK           0x70
#define MMDVM_NAK           0x7F

// DMR data/control
#define MMDVM_DMR_DATA1     0x18
#define MMDVM_DMR_LOST1     0x19
#define MMDVM_DMR_DATA2     0x1A
#define MMDVM_DMR_LOST2     0x1B
#define MMDVM_DMR_SHORTLC   0x1C
#define MMDVM_DMR_START     0x1D
#define MMDVM_DMR_ABORT     0x1E

// YSF data/control
#define MMDVM_YSF_DATA      0x20
#define MMDVM_YSF_LOST      0x21

// NXDN data/control
#define MMDVM_NXDN_DATA     0x40
#define MMDVM_NXDN_LOST     0x41

// MMDVM SET_MODE protocol codes (external values)
#define MMDVM_MODE_IDLE     0x00
#define MMDVM_MODE_DMR      0x02
#define MMDVM_MODE_YSF      0x03
#define MMDVM_MODE_NXDN     0x05

#define DMR_CONTROL_NONE  0x00
#define DMR_CONTROL_VOICE 0x20
#define DMR_CONTROL_DATA  0x40
#define DMR_CONTROL_IDLE  0x80
