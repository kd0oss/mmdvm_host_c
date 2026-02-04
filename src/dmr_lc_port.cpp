#include <cstring>
#include "dmr_lc_port.h"
#include "DMRDefines.h"
#include "DMRLC.h"
#include "DMRFullLC.h"

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
