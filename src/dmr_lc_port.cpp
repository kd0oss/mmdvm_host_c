#include <cstring>
#include "dmr_lc_port.h"
#include "log.h"

// MMDVMHost headers vendored under third_party/mmdvmhost
#include "DMRDefines.h"  // for FLCO enum (some CDMRLC APIs)
#include "DMRLC.h"       // CDMRLC itself
#include "DMRFullLC.h"

int dmr_lc_parse_ids(const uint8_t* header33, size_t len,
                     uint32_t* out_src_id, uint32_t* out_dst_id,
                     int* out_is_group) {
  if (!header33 || len < 33 || !out_src_id || !out_dst_id)
    return 0;

  CDMRFullLC fullLC;
  CDMRLC *lc = fullLC.decode(header33, DT_VOICE_LC_HEADER);
  if (lc == NULL)
    return 0;

  *out_src_id = lc->getSrcId();
  *out_dst_id = lc->getDstId();
//  if (out_is_group) *out_is_group = is_group;
  return 1;
}
