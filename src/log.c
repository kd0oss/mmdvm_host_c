#include "log.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#ifdef __STDC_NO_ATOMICS__
static volatile unsigned int g_rx_enabled = 0;
static volatile unsigned int g_tx_enabled = 0;
#else
#include <stdatomic.h>
static _Atomic unsigned int g_rx_enabled = 0;
static _Atomic unsigned int g_tx_enabled = 0;
#endif

static const char* type_name(uint8_t t) {
  switch (t) {
    case 0x18: return "DMR_DATA1";
    case 0x19: return "DMR_LOST1";
    case 0x1A: return "DMR_DATA2";
    case 0x1B: return "DMR_LOST2";
    case 0x1C: return "DMR_SHORTLC";
    case 0x1D: return "DMR_START";
    case 0x1E: return "DMR_ABORT";
    case 0x20: return "YSF_DATA";
    case 0x21: return "YSF_LOST";
    case 0x40: return "NXDN_DATA";
    case 0x41: return "NXDN_LOST";
    case 0x00: return "GET_VERSION";
    case 0x01: return "GET_STATUS";
    case 0x02: return "SET_CONFIG";
    case 0x03: return "SET_MODE";
    case 0x04: return "SET_FREQ";
    case 0x70: return "ACK";
    case 0x7F: return "NAK";
    default: return "UNKNOWN";
  }
}

static uint64_t now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

void log_set_rx_enabled(int enabled) {
#ifdef __STDC_NO_ATOMICS__
  g_rx_enabled = enabled ? 1U : 0U;
#else
  atomic_store_explicit(&g_rx_enabled, enabled ? 1U : 0U, memory_order_release);
#endif
}
void log_set_tx_enabled(int enabled) {
#ifdef __STDC_NO_ATOMICS__
  g_tx_enabled = enabled ? 1U : 0U;
#else
  atomic_store_explicit(&g_tx_enabled, enabled ? 1U : 0U, memory_order_release);
#endif
}

void hex_dump(const uint8_t* p, size_t n) {
  for (size_t i = 0; i < n; i++) {
    printf("%02X", p[i]);
    if (i + 1 < n) putchar(' ');
  }
}

void log_frame_rx(uint8_t type, const uint8_t* payload, size_t len) {
#ifdef __STDC_NO_ATOMICS__
  if (!g_rx_enabled) return;
#else
  if (atomic_load_explicit(&g_rx_enabled, memory_order_acquire) == 0U) return;
#endif
  uint64_t ts = now_ms();
  printf("[RX] ts=%" PRIu64 " type=0x%02X (%s) len=%zu payload=", ts, type, type_name(type), len);
  hex_dump(payload, len);
  putchar('\n');
  fflush(stdout);
}

void log_frame_tx(uint8_t type, const uint8_t* payload, size_t len) {
#ifdef __STDC_NO_ATOMICS__
//  if (!g_tx_enabled) return;
#else
//  if (atomic_load_explicit(&g_tx_enabled, memory_order_acquire) == 0U) return;
#endif
  uint64_t ts = now_ms();
  printf("[TX] ts=%" PRIu64 " type=0x%02X (%s) len=%zu payload=", ts, type, type_name(type), len);
  hex_dump(payload, len);
  putchar('\n');
  fflush(stdout);
}
