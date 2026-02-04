#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Strict check using MMDVMHost CSBK decode path (BPTC19696 + CRC + CSBKO)
int csbk_is_bsdwnact_strict(const uint8_t* coded, size_t len, uint32_t* out_srcId);

// Parsed CSBK information from CDMRCSBK::put()
typedef struct {
  uint8_t  csbko;    // CSBK opcode (0..63 from CSBKO enum)
  uint8_t  fid;      // Frame Information
  uint8_t  cbf;      // Colour Bits Field (when present)
  uint32_t srcId;    // Source ID (when present)
  uint32_t dstId;    // Destination ID (when present)
  uint8_t  valid;    // 1 if decoding and CRC check succeeded
} dmr_csbk_info_t;

// General parse. Returns 1 on success and fills 'out'; 0 on failure.
int csbk_parse_info(const uint8_t* coded, size_t len, dmr_csbk_info_t* out);

#ifdef __cplusplus
}
#endif
