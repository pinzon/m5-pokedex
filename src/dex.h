#pragma once
#include <stdint.h>

// Layout must match tools/build_dex.py (packed, little endian).
constexpr int DEX_SPRITE = 144;
constexpr uint8_t DEX_NO_TYPE = 0xFF;

struct __attribute__((packed)) DexRecord {
  uint16_t id;
  char name[12];    // not NUL-terminated when full
  char genus[24];
  uint8_t type1, type2;
  uint16_t height_dm, weight_hg;
  uint8_t stats[6];  // hp, atk, def, spa, spd, spe
  char flavor[240];
  uint8_t sprite[DEX_SPRITE * DEX_SPRITE / 8];  // MSB-first rows, 1 = black
};

bool dex_init();  // validates header; false if blob is missing/corrupt
uint16_t dex_count();
const DexRecord& dex_get(uint16_t idx);  // idx 0-based
const char* dex_type_name(uint8_t type);
// Copies a fixed-width field into buf as a NUL-terminated string.
const char* dex_str(const char* field, int len, char* buf);
