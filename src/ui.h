#pragma once
#include <stdint.h>

constexpr int LIST_ROWS = 10;

void ui_init();
void ui_set_quality(bool quality);  // true: slow full-quality refresh, false: fastest
void ui_draw_list(uint16_t cursor);
void ui_draw_jump(uint16_t cursor, const uint8_t digits[3], int active);
void ui_draw_detail(uint16_t idx, int page);
