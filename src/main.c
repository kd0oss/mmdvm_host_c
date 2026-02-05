#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "modem.h"
#include "mode.h"
#include "repeater_control.h"
#include "set_config.h"
#include "log.h"
#include "time_util.h"
#include "mode_dmr.h"   // <-- add this include

// Mode constructors
mode_handler_t* dmr_mode_new(void* ctx);
mode_handler_t* ysf_mode_new(void* ctx);
mode_handler_t* nxdn_mode_new(void* ctx);

extern void* rx_thread_fn(void* arg);
extern void* tx_thread_fn(void* arg);
extern void* status_thread_fn(void* arg);
extern void rc_tick(repeater_ctrl_t* rc, mode_handler_t* dmr, mode_handler_t* ysf, mode_handler_t* nxdn);
extern void set_rt(pthread_t thr, int prio, int cpu);

// Globals
modem_t* g_modem = NULL;
mode_handler_t* g_dmr = NULL;
mode_handler_t* g_ysf = NULL;
mode_handler_t* g_nxdn = NULL;
repeater_ctrl_t g_rc = {0};
config_t* g_cfg = NULL;

typedef struct {
  modem_t* modem;
  config_t* cfg;
  mode_handler_t* dmr;
  mode_handler_t* ysf;
  mode_handler_t* nxdn;
} app_ctx_t;

int main(int argc, char** argv) {
  const char* cfg_path = (argc > 1) ? argv[1] : "config.ini";
  config_t* cfg = config_load(cfg_path);
  if (!cfg) { fprintf(stderr, "Failed to load config\n"); return 1; }

  g_modem = modem_open(cfg->serial_dev ? cfg->serial_dev : "/dev/ttyACM0", cfg->baud ? cfg->baud : 460800);
  if (!g_modem) { fprintf(stderr, "Failed to open modem on %s\n", cfg->serial_dev ? cfg->serial_dev : "(null)"); config_free(cfg); return 1; }

  g_rc.mode = cfg->duplex;
  g_rc.tx_lockout_ms = 50;

  modem_get_version(g_modem);
  apply_modem_config(g_modem, cfg);

  app_ctx_t app = { .modem = g_modem, .cfg = cfg };
  g_dmr = app.dmr = dmr_mode_new(&app);
  g_ysf = app.ysf = ysf_mode_new(&app);
  g_nxdn = app.nxdn = nxdn_mode_new(&app);
  g_cfg = cfg;

  g_dmr->ops->init(g_dmr);
  g_ysf->ops->init(g_ysf);
  g_nxdn->ops->init(g_nxdn);

  g_dmr->hang_ms = cfg->dmr_hang_ms;
  g_ysf->hang_ms = cfg->ysf_hang_ms;
  g_nxdn->hang_ms = cfg->nxdn_hang_ms;

  log_set_rx_enabled(cfg->log_frames_rx);
  log_set_tx_enabled(cfg->log_frames_tx);

  rc_set_dmr_tx_on(&g_rc, 0);

  pthread_t rx_thr, tx_thr, st_thr;
  pthread_create(&rx_thr, NULL, rx_thread_fn, NULL);
  pthread_create(&tx_thr, NULL, tx_thread_fn, NULL);
  pthread_create(&st_thr, NULL, status_thread_fn, g_modem);

  set_rt(rx_thr, 40, 1);
  set_rt(tx_thr, 20, 2);
  set_rt(st_thr, 10, 0);

  for (;;) {
    rc_tick(&g_rc, g_dmr, g_ysf, g_nxdn);
    struct timespec req = {.tv_sec=0, .tv_nsec=2*1000*1000};
    nanosleep(&req, NULL);
  }

  pthread_join(rx_thr, NULL);
  pthread_join(tx_thr, NULL);
  pthread_join(st_thr, NULL);
  modem_close(g_modem);
  config_free(cfg);
  return 0;
}
