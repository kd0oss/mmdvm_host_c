#pragma once
#include <stdint.h>
#include <stddef.h>
#include "dmr_defines.h"

// Decodes 96-bit (12 byte) CSBK payload from 196-bit coded block.
// Returns 1 if a valid CSBK with opcode BSDWNACT, 0 otherwise.
// If valid, optionally returns srcId via out_srcId (nullable).
int csbk_is_bsdwnact(const uint8_t* coded, size_t len, uint32_t* out_srcId);
