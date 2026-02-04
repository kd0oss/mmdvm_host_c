#include "dmr_csbk_port.h"
// Vendored MMDVMHost headers
#include "DMRCSBK.h"

// Strict BSDWNACT detection using MMDVMHost's CDMRCSBK
int csbk_is_bsdwnact_strict(const uint8_t* coded, size_t len, uint32_t* out_srcId) {
  (void)len;
  if (!coded) return 0;

  CDMRCSBK csbk;
  if (!csbk.put(coded))
    return 0;

  // Check opcode: Downlink Activate (BSDWNACT)
  if (csbk.getCSBKO() != CSBKO::BSDWNACT)
    return 0;

  if (out_srcId)
    *out_srcId = csbk.getSrcId();

  return 1;
}

// General CSBK parse to expose CBF, IDs, etc.
int csbk_parse_info(const uint8_t* coded, size_t len, dmr_csbk_info_t* out) {
  (void)len;
  if (!coded || !out)
    return 0;

  CDMRCSBK csbk;
  if (!csbk.put(coded)) {
    out->valid = 0;
    return 0;
  }

  out->csbko = static_cast<uint8_t>(csbk.getCSBKO());
  out->fid   = csbk.getFID();
  out->cbf   = csbk.getCBF();
  out->srcId = csbk.getSrcId();
  out->dstId = csbk.getDstId();
  out->valid = 1;
  return 1;
}
