#pragma once
#include <stdint.h>
#include <stddef.h>

static inline void fill_dmr_test_frame(uint8_t frame33[33], uint8_t control) {
  for (size_t i = 0; i < 33; ++i) frame33[i] = (uint8_t)((i & 1) ? 0x55 : 0xAA);
  (void)control;
}

static inline void fill_ysf_test_frame(uint8_t frame120[120]) {
  for (size_t i = 0; i < 120; ++i) frame120[i] = (uint8_t)(i);
}

static inline void fill_nxdn_test_frame(uint8_t frame48[48]) {
  for (size_t i = 0; i < 48; ++i) frame48[i] = (uint8_t)(i * 5);
}
