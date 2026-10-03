#include "ui.h"

#include <M5Unified.h>

#include "dex.h"

static constexpr int W = 200;
static constexpr int HEADER_H = 20;
static constexpr int ROW_H = 18;
static constexpr int DETAIL_PAGES = 3;

static auto& gfx = M5.Display;

void ui_init() {
  gfx.setRotation(0);
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextWrap(false);
  ui_set_quality(true);
}

void ui_set_quality(bool quality) {
  gfx.setEpdMode(quality ? epd_mode_t::epd_quality : epd_mode_t::epd_fastest);
}

static void begin() {
  gfx.startWrite();
  gfx.fillScreen(TFT_WHITE);
  gfx.setFont(&fonts::Font2);
  gfx.setTextDatum(top_left);
}

static void end() { gfx.endWrite(); }

// left_max: width budget for the title; long names fall back to the smaller font.
static void header(const char* left, const char* right, int left_max = W - 8) {
  gfx.setFont(&fonts::FreeSansBold9pt7b);
  if (gfx.textWidth(left) > left_max) gfx.setFont(&fonts::Font2);
  gfx.drawString(left, 4, 1);
  gfx.setFont(&fonts::Font2);
  if (right) {
    gfx.setTextDatum(top_right);
    gfx.drawString(right, W - 4, 2);
    gfx.setTextDatum(top_left);
  }
  gfx.drawFastHLine(0, HEADER_H, W, TFT_BLACK);
}

static void detail_header(const DexRecord& r, int page) {
  char name[13], title[24];
  dex_str(r.name, sizeof(r.name), name);
  snprintf(title, sizeof(title), "#%03u %s", r.id, name);
  for (char* c = title; *c; ++c) *c = toupper(*c);
  header(title, nullptr, W - 44);
  // Page dots: filled = current.
  for (int i = 0; i < DETAIL_PAGES; ++i) {
    int x = W - 30 + i * 10, y = HEADER_H / 2;
    if (i == page) gfx.fillCircle(x, y, 3, TFT_BLACK);
    else gfx.drawCircle(x, y, 3, TFT_BLACK);
  }
}

static void list_row(uint16_t idx, bool sel) {
  const DexRecord& r = dex_get(idx);
  const int y = HEADER_H + 1 + idx % LIST_ROWS * ROW_H;
  gfx.fillRect(0, y, W, ROW_H, sel ? TFT_BLACK : TFT_WHITE);
  gfx.setTextColor(sel ? TFT_WHITE : TFT_BLACK, sel ? TFT_BLACK : TFT_WHITE);
  char buf[24], name[13];
  snprintf(buf, sizeof(buf), "%03u  %s", r.id, dex_str(r.name, sizeof(r.name), name));
  gfx.drawString(buf, 6, y + 1);
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
}

static void list_full(uint16_t cursor) {
  const uint16_t count = dex_count();
  const uint16_t top = cursor / LIST_ROWS * LIST_ROWS;
  char buf[24];
  begin();
  snprintf(buf, sizeof(buf), "%u/%u", top / LIST_ROWS + 1, (count + LIST_ROWS - 1) / LIST_ROWS);
  header("POKEDEX", buf);
  gfx.setFont(&fonts::Font2);
  for (uint16_t idx = top; idx < top + LIST_ROWS && idx < count; ++idx) list_row(idx, idx == cursor);
}

static void jump_box(const uint8_t digits[3], int active) {
  const int bw = 140, bh = 80, bx = (W - bw) / 2, by = 60;
  gfx.fillRect(bx, by, bw, bh, TFT_WHITE);
  gfx.drawRect(bx, by, bw, bh, TFT_BLACK);
  gfx.drawRect(bx + 2, by + 2, bw - 4, bh - 4, TFT_BLACK);
  gfx.setFont(&fonts::Font2);
  gfx.setTextDatum(top_center);
  gfx.drawString("GO TO #", W / 2, by + 8);
  gfx.setFont(&fonts::Font4);
  for (int i = 0; i < 3; ++i) {
    const int cx = W / 2 + (i - 1) * 28;
    char d[2] = {char('0' + digits[i]), 0};
    gfx.drawString(d, cx, by + 32);
    if (i == active) gfx.fillRect(cx - 9, by + 60, 18, 3, TFT_BLACK);
  }
  gfx.setTextDatum(top_left);
}

void ui_draw_list(uint16_t cursor) {
  list_full(cursor);
  end();
}

void ui_draw_list_move(uint16_t from, uint16_t to) {
  gfx.startWrite();
  gfx.setFont(&fonts::Font2);
  gfx.setTextDatum(top_left);
  list_row(from, false);
  list_row(to, true);
  end();
}

void ui_draw_jump(uint16_t cursor, const uint8_t digits[3], int active) {
  list_full(cursor);
  jump_box(digits, active);
  end();
}

void ui_draw_jump_box(const uint8_t digits[3], int active) {
  gfx.startWrite();
  jump_box(digits, active);
  end();
}

static void page_sprite(const DexRecord& r) {
  gfx.drawBitmap((W - DEX_SPRITE) / 2, HEADER_H + 2, r.sprite, DEX_SPRITE, DEX_SPRITE, TFT_BLACK);
  char types[24];
  if (r.type2 == DEX_NO_TYPE) snprintf(types, sizeof(types), "%s", dex_type_name(r.type1));
  else snprintf(types, sizeof(types), "%s / %s", dex_type_name(r.type1), dex_type_name(r.type2));
  gfx.setFont(&fonts::FreeSansBold9pt7b);
  gfx.setTextDatum(bottom_center);
  gfx.drawString(types, W / 2, 198);
  gfx.setTextDatum(top_left);
}

static void page_stats(const DexRecord& r) {
  static const char* const LABELS[6] = {"HP", "ATK", "DEF", "SPA", "SPD", "SPE"};
  char buf[32], genus[25];
  snprintf(buf, sizeof(buf), "%s Pokemon", dex_str(r.genus, sizeof(r.genus), genus));
  gfx.drawString(buf, 4, HEADER_H + 4);
  snprintf(buf, sizeof(buf), "HT %u.%u m   WT %u.%u kg", r.height_dm / 10, r.height_dm % 10,
           r.weight_hg / 10, r.weight_hg % 10);
  gfx.drawString(buf, 4, HEADER_H + 22);
  gfx.drawFastHLine(0, HEADER_H + 42, W, TFT_BLACK);

  const int bar_x = 66, bar_w = W - bar_x - 4;
  int total = 0;
  for (int i = 0; i < 6; ++i) {
    const int y = HEADER_H + 48 + i * 19;
    const uint8_t v = r.stats[i];
    total += v;
    gfx.drawString(LABELS[i], 4, y);
    gfx.setTextDatum(top_right);
    snprintf(buf, sizeof(buf), "%u", v);
    gfx.drawString(buf, bar_x - 6, y);
    gfx.setTextDatum(top_left);
    gfx.drawRect(bar_x, y + 3, bar_w, 10, TFT_BLACK);
    gfx.fillRect(bar_x, y + 3, v * bar_w / 255, 10, TFT_BLACK);
  }
  snprintf(buf, sizeof(buf), "TOTAL %d", total);
  gfx.setTextDatum(bottom_right);
  gfx.drawString(buf, W - 4, 199);
  gfx.setTextDatum(top_left);
}

// Greedy word wrap using the current font's real glyph widths.
static void page_flavor(const DexRecord& r) {
  char text[sizeof(r.flavor) + 1];
  dex_str(r.flavor, sizeof(r.flavor), text);
  const int max_w = W - 8, line_h = 17;
  int y = HEADER_H + 6;
  char line[64] = "";
  for (char* word = strtok(text, " "); word; word = strtok(nullptr, " ")) {
    char trial[64];
    snprintf(trial, sizeof(trial), line[0] ? "%s %s" : "%s%s", line, word);
    if (line[0] && gfx.textWidth(trial) > max_w) {
      gfx.drawString(line, 4, y);
      y += line_h;
      snprintf(line, sizeof(line), "%s", word);
    } else {
      snprintf(line, sizeof(line), "%s", trial);
    }
  }
  if (line[0]) gfx.drawString(line, 4, y);
}

void ui_draw_detail(uint16_t idx, int page) {
  const DexRecord& r = dex_get(idx);
  begin();
  detail_header(r, page);
  gfx.setFont(&fonts::Font2);
  switch (page) {
    case 0: page_sprite(r); break;
    case 1: page_stats(r); break;
    default: page_flavor(r); break;
  }
  end();
}
