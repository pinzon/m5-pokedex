#pragma once

enum class Input { NONE, UP, DOWN, UP_FAST, DOWN_FAST, PRESS, BACK, RANDOM };

void input_init();  // after M5.begin()
// Call once per loop after M5.update().
Input input_poll();
const char* input_name(Input in);
