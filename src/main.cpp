#include <M5Unified.h>
#include <Preferences.h>

#include "dex.h"
#include "input.h"
#include "ui.h"

enum class Screen : uint8_t { LIST, JUMP, DETAIL };

static constexpr uint32_t QUALITY_AFTER_MS = 1500;  // clean redraw to clear ghosting
static constexpr uint32_t POWER_OFF_AFTER_MS = 60000;
static constexpr int DETAIL_PAGES = 3;

static Preferences prefs;
static Screen screen = Screen::LIST;
static uint16_t cursor = 0;  // 0-based dex index
static uint8_t page = 0;
static uint8_t digits[3];
static int active_digit = 0;
static uint32_t last_input = 0;
static bool quality_pending = false;
static bool ready = false;

static void render() {
  switch (screen) {
    case Screen::LIST: ui_draw_list(cursor); break;
    case Screen::JUMP: ui_draw_jump(cursor, digits, active_digit); break;
    case Screen::DETAIL: ui_draw_detail(cursor, page); break;
  }
}

static void move(int delta) {
  const int n = dex_count();
  cursor = ((int)cursor + delta % n + n) % n;
}

static void save_state() {
  // JUMP is a transient overlay; resume on the list underneath it.
  const uint8_t s = (uint8_t)(screen == Screen::JUMP ? Screen::LIST : screen);
  if (prefs.getUChar("screen", 0xFF) != s) prefs.putUChar("screen", s);
  if (prefs.getUShort("cursor", 0xFFFF) != cursor) prefs.putUShort("cursor", cursor);
  if (prefs.getUChar("page", 0xFF) != page) prefs.putUChar("page", page);
}

static void load_state() {
  const uint8_t s = prefs.getUChar("screen", 0);
  screen = s == (uint8_t)Screen::DETAIL ? Screen::DETAIL : Screen::LIST;
  cursor = prefs.getUShort("cursor", 0);
  page = prefs.getUChar("page", 0);
  if (cursor >= dex_count()) cursor = 0;
  if (page >= DETAIL_PAGES) page = 0;
}

static void start_jump() {
  const unsigned n = dex_get(cursor).id;
  digits[0] = n / 100;
  digits[1] = n / 10 % 10;
  digits[2] = n % 10;
  active_digit = 0;
  screen = Screen::JUMP;
}

static void show_random() {
  const uint16_t n = dex_count();
  cursor = (cursor + 1 + esp_random() % (n - 1)) % n;  // never the current entry
  screen = Screen::DETAIL;
  page = 0;
}

static void handle_list(Input in) {
  switch (in) {
    case Input::UP: move(-1); break;
    case Input::DOWN: move(1); break;
    case Input::UP_FAST: move(-LIST_ROWS); break;
    case Input::DOWN_FAST: move(LIST_ROWS); break;
    case Input::PRESS: screen = Screen::DETAIL; page = 0; break;
    case Input::BACK: start_jump(); break;
    default: break;
  }
}

static void handle_jump(Input in) {
  uint8_t& d = digits[active_digit];
  switch (in) {
    case Input::UP: case Input::UP_FAST: d = (d + 1) % 10; break;
    case Input::DOWN: case Input::DOWN_FAST: d = (d + 9) % 10; break;
    case Input::BACK: screen = Screen::LIST; break;
    case Input::PRESS:
      if (++active_digit == 3) {
        const int n = constrain(digits[0] * 100 + digits[1] * 10 + digits[2], 1, (int)dex_count());
        cursor = n - 1;
        screen = Screen::LIST;
      }
      break;
    default: break;
  }
}

static void handle_detail(Input in) {
  switch (in) {
    case Input::UP: case Input::UP_FAST: move(-1); break;
    case Input::DOWN: case Input::DOWN_FAST: move(1); break;
    case Input::PRESS: page = (page + 1) % DETAIL_PAGES; break;
    case Input::BACK: screen = Screen::LIST; break;
    default: break;
  }
}

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);
  input_init();
  ui_init();

  if (!dex_init()) {
    Serial.println("dex.bin invalid");
    M5.Display.drawString("dex.bin invalid", 4, 4);
    return;
  }
  Serial.printf("dex: %u records\n", dex_count());

  prefs.begin("dex");
  load_state();
  render();  // quality mode from ui_init
  ui_set_quality(false);
  last_input = millis();
  ready = true;
}

void loop() {
  if (!ready) return delay(1000);
  M5.update();
  const uint32_t now = millis();
  const Input in = input_poll();

  if (in != Input::NONE) {
    Serial.printf("input %s\n", input_name(in));
    if (in == Input::RANDOM) show_random();
    else switch (screen) {
      case Screen::LIST: handle_list(in); break;
      case Screen::JUMP: handle_jump(in); break;
      case Screen::DETAIL: handle_detail(in); break;
    }
    render();
    last_input = now;
    quality_pending = true;
  } else if (quality_pending && now - last_input >= QUALITY_AFTER_MS) {
    ui_set_quality(true);
    render();
    ui_set_quality(false);
    save_state();
    quality_pending = false;
  } else if (now - last_input >= POWER_OFF_AFTER_MS) {
    Serial.println("idle: power off");
    Serial.flush();
    save_state();
    M5.Display.waitDisplay();
    M5.Power.powerOff();
    last_input = now;  // still running on USB power: wait another idle period
  }
  delay(10);
}
