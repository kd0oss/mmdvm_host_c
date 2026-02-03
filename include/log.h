#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void log_info(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

// Enable/disable per-direction frame logging
void log_set_rx_enabled(int enabled);
void log_set_tx_enabled(int enabled);

// Log helpers (internal use; call only if enabled)
void hex_dump(const uint8_t* p, size_t n);
void log_frame_rx(uint8_t type, const uint8_t* payload, size_t len);
void log_frame_tx(uint8_t type, const uint8_t* payload, size_t len);

#ifdef __cplusplus
}
#endif
