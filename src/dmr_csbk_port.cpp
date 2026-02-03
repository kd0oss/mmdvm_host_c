extern "C" {
#include "dmr_csbk_port.h"
}

#include <cstddef>
#include <cstdint>
#include <cstring>

// Vendored exact MMDVMHost headers
#include "BPTC19696.h"
#include "CRC.h"
#include "DMRCSBK.h"
#include "log.h"

// Reverse bit order in a byte (little <-> big endian bit order)
static inline uint8_t bitrev8(uint8_t x) {
  x = ((x & 0xF0U) >> 4) | ((x & 0x0FU) << 4);
  x = ((x & 0xCCU) >> 2) | ((x & 0x33U) << 2);
  x = ((x & 0xAAU) >> 1) | ((x & 0x55U) << 1);
  return x;
}

// Try decoding using MMDVMHost CSBK path over a 25-byte buffer
static int try_decode_csbk(const uint8_t* in25, uint32_t* out_srcId) {
  try {
    CDMRCSBK csbk;
    bool ok = csbk.put(in25);
    if (!ok)
      return 0;
    if (csbk.getCSBKO() != CSBKO::BSDWNACT)
      return 0;
    if (out_srcId)
      *out_srcId = csbk.getSrcId();
    return 1;
  } catch (...) {
    return 0;
  }
}

// Strict BSDWNACT with offset probing and bit-order fallback
// coded points at the start of the modem payload (meta likely at coded[0])
extern "C" int csbk_is_bsdwnact_strict(const uint8_t* coded, size_t len, uint32_t* out_srcId) {
//  if (!coded || len < 27) // need room for meta/meta2 + 25 coded bytes
  //  return 0;

  // Candidate starts: upstream (+2: [TAG][META][coded...]) and observed (+1: [META][META2][coded...])

  // Direct try (big-endian bit order per byte)
  if (try_decode_csbk(coded, out_srcId))
    return 1;

  return 0;
}
