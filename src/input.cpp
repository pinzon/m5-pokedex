#include "input.h"

#include <M5Unified.h>

// CoreInk in M5Unified: BtnA = G37 dial up, BtnB = G38 dial press, BtnC = G39 dial down,
// BtnEXT = G5 top button.
static constexpr uint32_t HOLD_MS = 600;
static constexpr uint32_t REPEAT_MS = 300;

static uint32_t next_repeat = 0;

static Input dial(m5::Button_Class& btn, Input step, Input fast) {
  const uint32_t now = millis();
  if (btn.wasPressed()) {
    next_repeat = now + HOLD_MS;
    return step;
  }
  if (btn.isPressed() && (int32_t)(now - next_repeat) >= 0) {
    next_repeat = now + REPEAT_MS;
    return fast;
  }
  return Input::NONE;
}

Input input_poll() {
  if (M5.BtnB.wasPressed()) return Input::PRESS;
  if (M5.BtnEXT.wasPressed()) return Input::BACK;
  Input in = dial(M5.BtnA, Input::UP, Input::UP_FAST);
  if (in != Input::NONE) return in;
  return dial(M5.BtnC, Input::DOWN, Input::DOWN_FAST);
}

const char* input_name(Input in) {
  switch (in) {
    case Input::UP: return "UP";
    case Input::DOWN: return "DOWN";
    case Input::UP_FAST: return "UP_FAST";
    case Input::DOWN_FAST: return "DOWN_FAST";
    case Input::PRESS: return "PRESS";
    case Input::BACK: return "BACK";
    default: return "NONE";
  }
}
