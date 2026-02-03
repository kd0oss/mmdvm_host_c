#include "dmr_csbk.h"
#include <string.h>

// If you have a real BPTC(196,96) decoder and CCITT-16 CRC, wire them here.
// Placeholder prototypes:
// int bptc19696_decode(const uint8_t* in196, uint8_t* out96); // return 0 on success
// int crc_ccitt162_check(const uint8_t* buf, size_t len);      // return 1 if valid

// Minimal placeholders: pass-through and "assume valid" (replace with real implementations)
static int bptc19696_decode(const uint8_t* in196, uint8_t* out96) {
  // TODO: replace with proper BPTC decode. For now, copy 12 bytes.
  memcpy(out96, in196, 12);
  return 0;
}

static int crc_ccitt162_check(const uint8_t* buf, size_t len) {
  // TODO: replace with actual CCITT-16 check.
  // Return 1 to accept; change to 0 to reject invalid.
  return 1;
}

int csbk_is_bsdwnact(const uint8_t* coded, size_t len, uint32_t* out_srcId) {
  if (!coded || len < 12) return 0;

  uint8_t data[12];
  if (bptc19696_decode(coded, data) != 0)
    return 0;

  // Apply mask to checksum bytes per MMDVMHost
  data[10] ^= CSBK_CRC_MASK[0];
  data[11] ^= CSBK_CRC_MASK[1];

  int valid = crc_ccitt162_check(data, 12);

  // Restore checksum bytes
  data[10] ^= CSBK_CRC_MASK[0];
  data[11] ^= CSBK_CRC_MASK[1];

  if (!valid) return 0;

  uint8_t opcode = data[0] & 0x3F;
  if (opcode != CSBKO_BSDWNACT) return 0;

  if (out_srcId) {
    uint32_t srcId = (data[7] << 16) | (data[8] << 8) | (data[9] << 0);
    *out_srcId = srcId;
  }
  return 1;
}
