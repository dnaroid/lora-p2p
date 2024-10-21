#ifndef GLOBAL_H
#define GLOBAL_H

#include "Arduino.h"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define SCREEN_ADDRESS  0x3C

#ifdef ESP32C6
#define PIN_BUTTON      11
#define PIN_LED         13
#define PIN_BUZZER      23
#define PIN_SDA         2
#define PIN_SCL         3
#define PIN_LORA_CS     22
#define PIN_LORA_IRQ    18
#define PIN_LORA_MOSI   20
#define PIN_LORA_MISO   21
#define PIN_LORA_SCK    19
#define PIN_LORA_RST    -1
#define MY_IDX          0
#define OTHER_IDX       1
#elifdef ESP32S3
#define PIN_BUTTON      4
#define PIN_BUZZER      42
#define PIN_LED         13
#define PIN_SDA         2
#define PIN_SCL         1
#define PIN_LORA_CS     38
#define PIN_LORA_IRQ    39
#define PIN_LORA_MOSI   36
#define PIN_LORA_MISO   35
#define PIN_LORA_SCK    37
#define PIN_LORA_RST    -1
#define MY_IDX          1
#define OTHER_IDX       0
#endif

#define LORA_SYNC_WORD  0x34
#define LORA_FRQ        868E6
#define ACK_TIMEOUT     2000
#define ACK_CHAR        "\x01"
#define PING_CHAR       "\x02"
#define PONG_CHAR       "\x03"
#define SEPARATOR       "|"
#define BUZZER_FRQ      4500
#define BUZZER_DURATION 200

#define STORAGE_SCOPE   "user_data"
#define STORAGE_USERS   "users"
#define STORAGE_LOGS    "logs"
#define STORAGE_NICKS   "foo,bar"

inline void print() {
}

template <typename T, typename... Args>
void print(T first, Args... args) {
  Serial.print(first);
  Serial.print(' ');
  print(args...);
}

template <typename... Args>
void println(Args... args) {
  print(args...);
  Serial.println();
}

#ifndef DEBUG
#define START_SERIAL
#define LOG(...)
#define LOGI(...)
#define LOGF(...)
#else
#define LOG(...) println(__VA_ARGS__)
#define LOGI(...) print(__VA_ARGS__)
#define LOGF(...) Serial.printf(__VA_ARGS__)
#define START_SERIAL int waitCount = 0;\
while (!Serial.available() && waitCount++ < 50) { \
Serial.begin(115200);\
delay(100);\
}\
Serial.println("--------------- started ---------------");
#endif


#endif //GLOBAL_H
