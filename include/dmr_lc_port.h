#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Decode IDs from LC header at 'header33' for the given data type (DT_VOICE_LC_HEADER or DT_DATA_HEADER).
int dmr_lc_parse_ids_dt(const uint8_t* header33, size_t len, uint8_t dt,
                        uint32_t* out_src_id, uint32_t* out_dst_id, int* out_is_group);

#ifdef __cplusplus
}
#endif
