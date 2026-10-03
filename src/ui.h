#pragma once
#include <stdint.h>

constexpr int LIST_ROWS = 10;

void ui_init();
void ui_set_quality(bool quality);  // true: slow full-quality refresh, false: fastest
// Full redraws refresh the whole panel; the others draw (and so refresh) only what changed.
void ui_draw_list(uint16_t cursor);
void ui_draw_list_move(uint16_t from, uint16_t to);  // both on the same list page
void ui_draw_jump(uint16_t cursor, const uint8_t digits[3], int active);  // list + box
void ui_draw_jump_box(const uint8_t digits[3], int active);  // box only, over the list
void ui_draw_detail(uint16_t idx, int page);
