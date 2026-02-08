#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parsed CSBK information from CDMRCSBK::put()
typedef struct {
  uint8_t  csbko;    // CSBK opcode (0..63 from CSBKO enum)
  uint8_t  fid;      // Frame Information
  uint8_t  cbf;      // Colour Bits Field (when present)
  uint32_t srcId;    // Source ID (when present)
  uint32_t dstId;    // Destination ID (when present)
  uint8_t  valid;    // 1 if decoding and CRC check succeeded
} dmr_csbk_info_t;

// Decode IDs from LC header at 'header33' for the given data type (DT_VOICE_LC_HEADER or DT_DATA_HEADER).
int dmr_lc_parse_ids_dt(const uint8_t* header33, size_t len, uint8_t dt,
                        uint32_t* out_src_id, uint32_t* out_dst_id, int* out_is_group);

int csbk_is_bsdwnact_strict(const uint8_t *coded, size_t len, uint32_t *out_srcId);
int csbk_parse_info(const uint8_t *coded, size_t len, dmr_csbk_info_t *out) ;

int dmr_get_color_code(const uint8_t* header33, size_t len, uint8_t* cc);
int dmr_get_data_type(const uint8_t* header33, size_t len, uint8_t* dt);

#ifdef __cplusplus
}
#endif
