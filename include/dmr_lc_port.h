#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse a 33-byte DMR RF voice header (the block after the two metadata bytes).
// Returns 1 on success and fills out_src_id/out_dst_id.
// Optionally sets out_is_group (1 group call, 0 private) if non-NULL.
// This adapter uses MMDVMHost's CDMRLC internally and ONLY returns IDs.
int dmr_lc_parse_ids(const uint8_t* header33, size_t len,
                     uint32_t* out_src_id, uint32_t* out_dst_id,
                     int* out_is_group);

#ifdef __cplusplus
}
#endif
