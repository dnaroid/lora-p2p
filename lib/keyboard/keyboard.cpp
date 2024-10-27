#include "keyboard.h"
#include <global.h>

#ifdef PIN_A0

#define DEBOUNCE              20
#define REPEAT_INTERVAL       10
#define INITIAL_REPEAT_DELAY  500

#define MOD_1 (char)(255)
#define MOD_3 (char)(254)
#define MOD_2 (char)(253)

#define UP (char)(252)
#define DOWN (char)(251)
#define LEFT (char)(250)
#define RIGHT (char)(249)
#define BREAK (char)(248)
#define BACKSPACE (char)(8)
#define ENTER (char)(13)
#define ESC (char)(27)


#define ROWS 6
#define COLS 5
#define MODS 2
#define MOD_1_ROW_OFFSET (1 * ROWS)
#define MOD_2_ROW_OFFSET   (2 * ROWS)

constexpr int rows[ROWS] = {PIN_A0, PIN_A1, PIN_A2, PIN_A3, PIN_A4, PIN_A5};
constexpr int cols[COLS] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4}; // D0-D10
char keyMap[ROWS * MODS][COLS] = {
  {'Q', 'W', 'E', 'R', 'T'}, {'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G'}, {'H', 'J', 'K', 'L', BACKSPACE},
  {'Z', 'X', 'C', 'V', 'B'}, {'N', 'M', ' ', MOD_1, ENTER},

  {'1', '2', '3', '4', '5'}, {'6', '7', '8', '9', '0'},
  {'!', '@', '#', '$', '%'}, {'(', ')', ',', '"', BACKSPACE},
  {'+', '-', '/', '*', '='}, {'?', ':', '.', MOD_1, ENTER},
};


char lastKey = 0;
unsigned long lastKeyTime = 0;
bool isRepeating = false;
int lastPressedRow = 0;
int lastPressedCol = 0;
int rowOffset = 0;
bool lockRowOffset = false;
volatile bool requestActivated = false;

static char scanKeypad() {
  for (int row = 0; row < ROWS; row++) {
    pinMode(rows[row], OUTPUT);
    digitalWrite(rows[row], LOW);

    for (int col = 0; col < COLS; col++) {
      pinMode(cols[col], INPUT_PULLUP);
      if (digitalRead(cols[col]) == LOW) {
        digitalWrite(rows[row], HIGH);
        pinMode(rows[row], INPUT);
        lastPressedRow = row;
        lastPressedCol = col;
        return keyMap[row][col];
      }
    }
    digitalWrite(rows[row], HIGH);
    pinMode(rows[row], INPUT);
  }
  return 0;
}

static void toggleLockMode() {
  if (lockRowOffset) {
    lockRowOffset = false;
    rowOffset = false;
  } else {
    lockRowOffset = true;
  }
}

static void toggleRowOffset(const int offset) {
  if (rowOffset == offset) {
    rowOffset = 0;
  } else {
    rowOffset = offset;
  }
  lockRowOffset = false;
}

static void toggleMode(const char key) {
  if (key == MOD_1) {
    toggleRowOffset(MOD_1_ROW_OFFSET);
  } else if (key == MOD_2) {
    toggleRowOffset(MOD_2_ROW_OFFSET);
  }
}

static bool isModKey(const char key) {
  return key == MOD_1 || key == MOD_2 || key == MOD_3;
}

static bool isPressed() {
  const unsigned long currentTime = millis();
  const char key = scanKeypad();

  if (key != 0) {
    delay(DEBOUNCE);
    const char debounceKey = scanKeypad();

    if (debounceKey == key) {
      if (key != lastKey) { // new key
        lastKey = key;
        lastKeyTime = currentTime;
        isRepeating = false;
        if (isModKey(key)) {
          toggleMode(key);
          return false;
        }
        return true;
      }

      if (!isRepeating && (currentTime - lastKeyTime >= INITIAL_REPEAT_DELAY)) { // start repeating
        lastKeyTime = currentTime;
        isRepeating = true;
        if (isModKey(key)) {
          toggleLockMode();
          return false;
        }
        return true;
      }

      if (isRepeating && (currentTime - lastKeyTime >= REPEAT_INTERVAL)) { // repeating
        lastKeyTime = currentTime;
        return !isModKey(key);
      }
    }
  } else { // key released
    lastKey = 0;
    lastKeyTime = 0;
  }
  return false;
}

char getKeyPressed() {
  if (isPressed()) {
    const auto key = keyMap[lastPressedRow + rowOffset][lastPressedCol];
    if (!lockRowOffset) rowOffset = 0;
    return key;
  }
  return 0;
}

void setupKeyboard() {
  for (const int row : rows) pinMode(row, INPUT);
}

#endif
