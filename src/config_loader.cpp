#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "config.h"
}

static inline std::string trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}
static inline std::string lower(const std::string& s) {
  std::string r = s;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return std::tolower(c); });
  return r;
}
static inline bool is_true(const std::string& v) {
  std::string t = lower(trim(v));
  return t == "1" || t == "true" || t == "yes" || t == "on";
}
static inline bool is_false(const std::string& v) {
  std::string t = lower(trim(v));
  return t == "0" || t == "false" || t == "no" || t == "off";
}

static inline int to_int(const std::string& v, int def) { try { return std::stoi(trim(v)); } catch (...) { return def; } }
static inline uint32_t to_u32(const std::string& v, uint32_t def) { try { return static_cast<uint32_t>(std::stoul(trim(v))); } catch (...) { return def; } }
static inline char* dup_cstr(const std::string& s) { char* p = (char*)std::malloc(s.size() + 1); if (!p) return nullptr; std::memcpy(p, s.c_str(), s.size() + 1); return p; }

config_t* config_load(const char* path) {
  std::ifstream in(path ? path : "");
  std::string section;
  std::string serial_dev = "/dev/ttyACM0";
  int baud = 460800;
  duplex_mode_t duplex = DUPLEX_FULL;
  int dmr_color_code = 1, dmr_allow_slot1 = 1, dmr_allow_slot2 = 1;
  uint32_t dmr_hang_ms = 3000;
  uint32_t ysf_node_id = 12345, ysf_hang_ms = 2000;
  int nxdn_ran = 1; uint32_t nxdn_hang_ms = 2000;
  int log_frames_rx = 0, log_frames_tx = 0;
  int echo_dmr = 1, echo_ysf = 0, echo_nxdn = 0;

  if (in.good()) {
    std::string line;
    while (std::getline(in, line)) {
      line = trim(line);
      if (line.empty() || line[0] == ';' || line[0] == '#') continue;
      if (line.front() == '[' && line.back() == ']') { section = lower(trim(line.substr(1, line.size() - 2))); continue; }
      auto pos = line.find('=');
      if (pos == std::string::npos) continue;
      std::string key = lower(trim(line.substr(0, pos)));
      std::string val = trim(line.substr(pos + 1));

      if (section == "system") {
        if (key == "duplex") duplex = (lower(val) == "full") ? DUPLEX_FULL : DUPLEX_HALF;
        else if (key == "serial_dev") serial_dev = val;
        else if (key == "baud") baud = to_int(val, baud);
        else if (key == "log_frames_rx") log_frames_rx = is_true(val) ? 1 : is_false(val) ? 0 : log_frames_rx;
        else if (key == "log_frames_tx") log_frames_tx = is_true(val) ? 1 : is_false(val) ? 0 : log_frames_tx;
        else if (key == "echo_dmr")  echo_dmr  = is_true(val) ? 1 : is_false(val) ? 0 : echo_dmr;
        else if (key == "echo_ysf")  echo_ysf  = is_true(val) ? 1 : is_false(val) ? 0 : echo_ysf;
        else if (key == "echo_nxdn") echo_nxdn = is_true(val) ? 1 : is_false(val) ? 0 : echo_nxdn;
      } else if (section == "dmr") {
        if (key == "color_code") dmr_color_code = to_int(val, dmr_color_code);
        else if (key == "allow_slot1") dmr_allow_slot1 = is_true(val) ? 1 : is_false(val) ? 0 : dmr_allow_slot1;
        else if (key == "allow_slot2") dmr_allow_slot2 = is_true(val) ? 1 : is_false(val) ? 0 : dmr_allow_slot2;
        else if (key == "hang_ms") dmr_hang_ms = to_u32(val, dmr_hang_ms);
      } else if (section == "ysf") {
        if (key == "node_id") ysf_node_id = to_u32(val, ysf_node_id);
        else if (key == "hang_ms") ysf_hang_ms = to_u32(val, ysf_hang_ms);
      } else if (section == "nxdn") {
        if (key == "ran") nxdn_ran = to_int(val, nxdn_ran);
        else if (key == "hang_ms") nxdn_hang_ms = to_u32(val, nxdn_hang_ms);
      }
    }
  }

  config_t* cfg = (config_t*)std::calloc(1, sizeof(config_t));
  if (!cfg) return nullptr;
  cfg->serial_dev = dup_cstr(serial_dev);
  cfg->baud = baud;
  cfg->duplex = duplex;
  cfg->dmr_color_code = dmr_color_code;
  cfg->dmr_allow_slot1 = dmr_allow_slot1;
  cfg->dmr_allow_slot2 = dmr_allow_slot2;
  cfg->dmr_hang_ms = dmr_hang_ms;
  cfg->ysf_node_id = ysf_node_id;
  cfg->ysf_hang_ms = ysf_hang_ms;
  cfg->nxdn_ran = nxdn_ran;
  cfg->nxdn_hang_ms = nxdn_hang_ms;
  cfg->log_frames_rx = log_frames_rx;
  cfg->log_frames_tx = log_frames_tx;
  cfg->echo_dmr  = echo_dmr;
  cfg->echo_ysf  = echo_ysf;
  cfg->echo_nxdn = echo_nxdn;
  return cfg;
}

void config_free(config_t* cfg) {
  if (!cfg) return;
  if (cfg->serial_dev) std::free(cfg->serial_dev);
  std::free(cfg);
}
