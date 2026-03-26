#include <Arduino.h>
#include "hw_config.h"

#define NES_A 0x01
#define NES_B 0x02
#define NES_SELECT 0x04
#define NES_START 0x08
#define NES_UP 0x10
#define NES_DOWN 0x20
#define NES_LEFT 0x40
#define NES_RIGHT 0x80

void setup_controller() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
}

extern "C" int IRAM_ATTR nes_get_gamepad_state() {
  int state = 0;

  if (digitalRead(BTN_A) == LOW) state |= NES_A;
  if (digitalRead(BTN_B) == LOW) state |= NES_B;
  if (digitalRead(BTN_SELECT) == LOW) state |= NES_SELECT;
  if (digitalRead(BTN_START) == LOW) state |= NES_START;
  if (digitalRead(BTN_UP) == LOW) state |= NES_UP;
  if (digitalRead(BTN_DOWN) == LOW) state |= NES_DOWN;
  if (digitalRead(BTN_LEFT) == LOW) state |= NES_LEFT;
  if (digitalRead(BTN_RIGHT) == LOW) state |= NES_RIGHT;

  return state;
}