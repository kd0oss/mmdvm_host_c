#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Monotonic time in milliseconds from CLOCK_MONOTONIC.
uint64_t now_ms(void);

#ifdef __cplusplus
}
#endif
