#include "dmr_ports.h"
// Vendored MMDVMHost headers
#include "DMRCSBK.h"
#include "DMRDefines.h"
#include "DMRLC.h"
#include "DMRFullLC.h"
#include "DMRSlotType.h"


// Strict BSDWNACT detection using MMDVMHost's CDMRCSBK
int csbk_is_bsdwnact_strict(const uint8_t *coded, size_t len, uint32_t *out_srcId) {
  (void)len;
  if (!coded)
    return 0;

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
int csbk_parse_info(const uint8_t *coded, size_t len, dmr_csbk_info_t *out) {
  (void)len;
  if (!coded || !out)
    return 0;

  CDMRCSBK csbk;
  if (!csbk.put(coded)) {
    out->valid = 0;
    return 0;
  }

  out->csbko = static_cast<uint8_t>(csbk.getCSBKO());
  out->fid = csbk.getFID();
  out->cbf = csbk.getCBF();
  out->srcId = csbk.getSrcId();
  out->dstId = csbk.getDstId();
  out->valid = 1;
  return 1;
}

int dmr_lc_parse_ids_dt(const uint8_t* header33, size_t len, uint8_t dt,
                        uint32_t* out_src_id, uint32_t* out_dst_id, int* out_is_group) {
  if (!header33 || len < 33 || !out_src_id || !out_dst_id) return 0;
  if (dt != DT_VOICE_LC_HEADER && dt != DT_DATA_HEADER) return 0;

  CDMRFullLC fullLC;
  CDMRLC* lc = fullLC.decode(header33, dt);
  if (lc == nullptr) return 0;

  *out_src_id = lc->getSrcId();
  *out_dst_id = lc->getDstId();
  // MMDVMHost CDMRLC carries FLCO; you can derive group/private from that if needed
  if (out_is_group) *out_is_group = (lc->getFLCO() == FLCO_GROUP) ? 1 : 0;
  return 1;
}

int dmr_get_color_code(const uint8_t* header33, size_t len, uint8_t* cc)
{
  if (!header33 || len < 33 || !cc)
    return 0;

  CDMRSlotType st;
  st.putData(header33);

  *cc = st.getColorCode();
  return 1;
}

int dmr_get_data_type(const uint8_t* header33, size_t len, uint8_t* dt)
{
  if (!header33 || len < 33 || !dt)
    return 0;

  CDMRSlotType st;
  st.putData(header33);

  *dt = st.getDataType();
  return 1;
}
