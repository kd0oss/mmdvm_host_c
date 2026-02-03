#include "modem.h"
#include "mmdvm_proto.h"
#include "log.h"
#include "time_util.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

struct modem { int fd; int baud; };

static int set_interface_attribs(int fd, int speed) {
  struct termios tty;
  if (tcgetattr(fd, &tty) != 0) return -1;
  cfmakeraw(&tty);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_iflag &= ~(IXON | IXOFF);

  speed_t sp = B115200;
#ifdef B460800
  if (speed == 460800) sp = B460800;
#endif
#ifdef B230400
  if (speed == 230400) sp = B230400;
#endif
  if (speed == 115200) sp = B115200;
  if (speed != 115200
#ifdef B230400
      && speed != 230400
#endif
#ifdef B460800
      && speed != 460800
#endif
      ) {
    fprintf(stderr, "Warning: requested baud %d not supported, falling back to 115200\n", speed);
  }

  cfsetospeed(&tty, sp); cfsetispeed(&tty, sp);
  tty.c_cc[VMIN] = 0; tty.c_cc[VTIME] = 1;
  return tcsetattr(fd, TCSANOW, &tty);
}

modem_t* modem_open(const char* dev, int baud) {
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) return NULL;
  if (set_interface_attribs(fd, baud) != 0) { close(fd); return NULL; }
  modem_t* m = calloc(1, sizeof(*m)); m->fd = fd; m->baud = baud; return m;
}
void modem_close(modem_t* m) { if (!m) return; if (m->fd >= 0) close(m->fd); free(m); }
int modem_fd(const modem_t* m) { return m ? m->fd : -1; }

static ssize_t read_n(int fd, uint8_t* buf, size_t n, int timeout_ms) {
  size_t rd = 0; uint64_t deadline = now_ms() + (timeout_ms > 0 ? timeout_ms : 0);
  while (rd < n) {
    ssize_t r = read(fd, buf + rd, n - rd);
    if (r > 0) { rd += (size_t)r; continue; }
    if (r == 0 || (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
      if (timeout_ms <= 0 || now_ms() > deadline) break;
      struct timespec req = {.tv_sec=0, .tv_nsec=1*1000*1000}; nanosleep(&req, NULL);
      continue;
    }
    return -1;
  }
  return (ssize_t)rd;
}

int modem_read_frame(modem_t* m, modem_frame_t* out) {
  if (!m || !out) return -1;
  uint8_t sof;
  for (;;) {
    ssize_t r = read(m->fd, &sof, 1);
    if (r == 1 && sof == MMDVM_SOF) break;
    if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return -1;
    struct timespec req = {.tv_sec=0, .tv_nsec=1*1000*1000}; nanosleep(&req, NULL);
  }
  uint8_t len = 0, type = 0;
  if (read_n(m->fd, &len, 1, 10) != 1) return -1;
  if (read_n(m->fd, &type, 1, 10) != 1) return -1;

  size_t pay_len = (len >= 3) ? (size_t)(len - 3) : 0;
  uint8_t* payload = (pay_len ? (uint8_t*)malloc(pay_len) : NULL);
  if (pay_len && (!payload || read_n(m->fd, payload, pay_len, 20) != (ssize_t)pay_len)) { free(payload); return -1; }

  out->ts_ms = now_ms();
  out->type  = type;

  switch (type) {
    case MMDVM_DMR_DATA1:
    case MMDVM_DMR_DATA2: out->mode = MODE_DMR; break;
    case MMDVM_YSF_DATA:  out->mode = MODE_YSF; break;
    case MMDVM_NXDN_DATA: out->mode = MODE_NXDN; break;
    default:
         log_frame_rx(type, payload, pay_len);
         free(payload);
         return 1;
  }
  out->data = payload; out->len = pay_len;

  // Log by envelope type; data/control frames both logged
  log_frame_rx(type, payload, pay_len);
  return 0;
}

static ssize_t write_n(int fd, const uint8_t* buf, size_t n) {
  size_t wr = 0;
  while (wr < n) {
    ssize_t w = write(fd, buf + wr, n - wr);
    if (w > 0) { wr += (size_t)w; continue; }
    if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
    if (w < 0) return -1;
    break;
  }
  return (ssize_t)wr;
}

int modem_write_envelope(modem_t* m, uint8_t type, const uint8_t* payload, size_t len) {
  if (!m) return -1;
  uint8_t header[3] = { MMDVM_SOF, (uint8_t)(3 + (payload ? len : 0)), type };
  log_frame_tx(type, payload ? payload : (const uint8_t*)"", payload ? len : 0);

  if (write_n(m->fd, header, sizeof(header)) != (ssize_t)sizeof(header)) return -1;
  if (payload && len) {
    if (write_n(m->fd, payload, len) != (ssize_t)len) return -1;
  }
  return 0;
}

int modem_write_frame(modem_t* m, radio_mode_t mode, const uint8_t* data, size_t len) {
  if (!m || !data || len == 0) return -1;
  switch (mode) {
    case MODE_DMR:  return modem_write_envelope(m, MMDVM_DMR_DATA1, data, len);
    case MODE_YSF:  return modem_write_envelope(m, MMDVM_YSF_DATA,  data, len);
    case MODE_NXDN: return modem_write_envelope(m, MMDVM_NXDN_DATA, data, len);
    default: return -1;
  }
}

int modem_get_version(modem_t* m) { return modem_write_envelope(m, MMDVM_GET_VERSION, NULL, 0); }
int modem_get_status(modem_t* m)  { return modem_write_envelope(m, MMDVM_GET_STATUS, NULL, 0); }

// Map internal radio_mode_t to MMDVM SET_MODE protocol codes.
static uint8_t mmdvm_mode_code(uint8_t internal_mode) {
  switch (internal_mode) {
    case MODE_DMR:  return MMDVM_MODE_DMR;
    case MODE_YSF:  return MMDVM_MODE_YSF;
    case MODE_NXDN: return MMDVM_MODE_NXDN;
    default:        return MMDVM_MODE_IDLE;
  }
}

int modem_set_mode(modem_t* m, uint8_t state)
{
  if (!m) return -1;
  uint8_t code = mmdvm_mode_code(state);
  uint8_t p[1] = { code };
  return modem_write_envelope(m, MMDVM_SET_MODE, p, 1);
}

int modem_dmr_start(modem_t* m, uint8_t start)
{
   if (!m) return -1;
   uint8_t p[1] = { start ? 0x01 : 0x00 };
   log_info("[TX] DMR_START tx_on=%u", p[0]);
   return modem_write_envelope(m, MMDVM_DMR_START, p, 1);
}

int modem_dmr_abort(modem_t* m, uint8_t slot) { uint8_t p[1] = { (slot == 2) ? 0x02 : 0x01 }; return modem_write_envelope(m, MMDVM_DMR_ABORT, p, 1); }
int modem_dmr_shortlc(modem_t* m, const uint8_t* slc9) { return modem_write_envelope(m, MMDVM_DMR_SHORTLC, slc9, 9); }
