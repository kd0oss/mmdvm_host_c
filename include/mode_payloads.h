#pragma once
#include <stdint.h>
#include <string.h>

#define DMR_FRAME_LENGTH_BYTES   33
#define YSF_FRAME_LENGTH_BYTES   120
#define NXDN_FRAME_LENGTH_BYTES  48

static inline size_t dmr_build_payload(uint8_t control, const uint8_t frame33[DMR_FRAME_LENGTH_BYTES], uint8_t out[DMR_FRAME_LENGTH_BYTES + 1]) {
  out[0] = control; memcpy(out + 1, frame33, DMR_FRAME_LENGTH_BYTES); return DMR_FRAME_LENGTH_BYTES + 1;
}
static inline size_t ysf_build_payload(uint8_t control, const uint8_t frame120[YSF_FRAME_LENGTH_BYTES], uint8_t out[YSF_FRAME_LENGTH_BYTES + 1]) {
  out[0] = control; memcpy(out + 1, frame120, YSF_FRAME_LENGTH_BYTES); return YSF_FRAME_LENGTH_BYTES + 1;
}
static inline size_t nxdn_build_payload(uint8_t control, const uint8_t frame48[NXDN_FRAME_LENGTH_BYTES], uint8_t out[NXDN_FRAME_LENGTH_BYTES + 1]) {
  out[0] = control; memcpy(out + 1, frame48, NXDN_FRAME_LENGTH_BYTES); return NXDN_FRAME_LENGTH_BYTES + 1;
}
