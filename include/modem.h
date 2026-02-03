#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct modem modem_t;

typedef enum {
  MODE_DMR = 0,  // internal enumerations; modem_set_mode maps to MMDVM codes
  MODE_YSF = 1,
  MODE_NXDN = 2,
} radio_mode_t;

typedef struct {
  radio_mode_t mode;
  uint8_t type;         // envelope TYPE (e.g., 0x18, 0x1A, 0x20, 0x40)
  const uint8_t* data;
  size_t len;
  uint64_t ts_ms;
} modem_frame_t;

modem_t* modem_open(const char* dev, int baud);
void     modem_close(modem_t* m);
int      modem_fd(const modem_t* m);

int      modem_read_frame(modem_t* m, modem_frame_t* out);
int      modem_write_envelope(modem_t* m, uint8_t type, const uint8_t* payload, size_t len);
int      modem_write_frame(modem_t* m, radio_mode_t mode, const uint8_t* data, size_t len);

int      modem_get_version(modem_t* m);
int      modem_get_status(modem_t* m);   // NEW: declare status polling API
int      modem_set_mode(modem_t* m, uint8_t state);

int      modem_dmr_start(modem_t* m, uint8_t start);
int      modem_dmr_abort(modem_t* m, uint8_t slot);
int      modem_dmr_shortlc(modem_t* m, const uint8_t* slc9);
