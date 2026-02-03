#pragma once
#include <stdint.h>
#include <stddef.h>

// Strict check using MMDVMHost CSBK decode path (BPTC19696 + CRC + CSBKO)
int csbk_is_bsdwnact_strict(const uint8_t* coded, size_t len, uint32_t* out_srcId);
