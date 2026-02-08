#include "set_config.h"
#include "mmdvm_proto.h"
#include <string.h>
#include <stdio.h>
#include "log.h"

static inline uint8_t clamp8u(int v) { if (v < 0) return 0; if (v > 255) return 255; return (uint8_t)v; }

int apply_modem_config(modem_t* m, const config_t* cfg) {
  if (!m || !cfg) return -1;

  uint8_t p[37];
  memset(p, 0, sizeof(p));

  uint8_t flags0 = 0x02;
  if (cfg->duplex == DUPLEX_HALF) flags0 |= 0x80;
  p[0] = flags0;

  uint8_t flags1 = 0x00;
  flags1 |= 0x02; // DMR
//  flags1 |= 0x04; // YSF
//  flags1 |= 0x10; // NXDN
  p[1] = flags1;

  p[2] = 0x00;
  p[3] = 10; // txDelay
  p[4] = 0;  // STATE_IDLE

  p[5] = 128; // txDCOffset
  p[6] = 128; // rxDCOffset

  p[7]  = clamp8u(50 * 2.55F + 0.5F);
  p[8]  = clamp8u(70);
  p[9]  = clamp8u(70);
  p[10] = clamp8u(70 * 2.55F + 0.5F);
  p[11] = clamp8u(70);
  p[12] = clamp8u(70);
  p[13] = clamp8u(70);
  p[14] = clamp8u(70);
  p[15] = clamp8u(70);
  p[16] = clamp8u(70);
  p[17] = clamp8u(70);

  p[20] = clamp8u((int)(cfg->ysf_hang_ms / 100));
  p[21] = clamp8u(20);
  p[22] = clamp8u((int)(cfg->nxdn_hang_ms / 100));
  p[23] = clamp8u(20);

  p[26] = (uint8_t)(cfg->dmr_color_code & 0x0F);
  p[27] = 0;

  p[28] = 128;
  p[29] = 0;
  p[30] = 0;
  p[31] = 0;

  p[32] = p[33] = p[34] = p[35] = p[36] = 0;
hex_dump(p, 37);
  int rc = modem_write_envelope(m, MMDVM_SET_CONFIG, p, sizeof(p));
  if (rc != 0) fprintf(stderr, "SET_CONFIG send failed\n");
  return rc;
}
