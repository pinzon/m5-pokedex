#include "dex.h"

#include <string.h>

extern const uint8_t dex_bin_start[] asm("_binary_data_dex_bin_start");
extern const uint8_t dex_bin_end[] asm("_binary_data_dex_bin_end");

struct __attribute__((packed)) DexHeader {
  char magic[4];
  uint16_t count;
  uint16_t record_size;
  uint8_t reserved[8];
};

static const DexHeader* header() { return reinterpret_cast<const DexHeader*>(dex_bin_start); }

bool dex_init() {
  const DexHeader* h = header();
  if (memcmp(h->magic, "DEX1", 4) != 0 || h->record_size < sizeof(DexRecord)) return false;
  return sizeof(DexHeader) + (size_t)h->count * h->record_size <= (size_t)(dex_bin_end - dex_bin_start);
}

uint16_t dex_count() { return header()->count; }

const DexRecord& dex_get(uint16_t idx) {
  const uint8_t* p = dex_bin_start + sizeof(DexHeader) + (size_t)idx * header()->record_size;
  return *reinterpret_cast<const DexRecord*>(p);
}

static const char* const TYPE_NAMES[] = {
    "NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL",
    "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK", "FAIRY"};

const char* dex_type_name(uint8_t type) {
  return type < sizeof(TYPE_NAMES) / sizeof(*TYPE_NAMES) ? TYPE_NAMES[type] : "";
}

const char* dex_str(const char* field, int len, char* buf) {
  memcpy(buf, field, len);
  buf[len] = '\0';
  return buf;
}
