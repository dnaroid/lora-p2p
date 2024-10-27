#ifndef GLOBAL_H
#define GLOBAL_H

#include "Arduino.h"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define SCREEN_ADDRESS  0x3C

#define MESSAGES_FILE   "/msg.txt"

#define IRQ_PIN_BITMASK(GPIO) (1ULL << GPIO)

#define PIN_BUZZER      23
#define PIN_SDA         18
#define PIN_SCL         15
#define PIN_LORA_CS     22
#define PIN_LORA_IRQ    5
#define PIN_LORA_MOSI   20
#define PIN_LORA_MISO   21
#define PIN_LORA_SCK    19

// #define PIN_BUTTON      13

#ifndef PIN_BUTTON
#define PIN_A0          6
#define PIN_A1          7
#define PIN_A2          8
#define PIN_A3          10
#define PIN_A4          11
#define PIN_A5          9
// #define PIN_D0          0
// #define PIN_D1          1
// #define PIN_D2          2
// #define PIN_D3          3
// #define PIN_D4          4
#define PIN_D4          0
#define PIN_D3          1
#define PIN_D2          2
#define PIN_D1          3
#define PIN_D0          4
#endif

#define LORA_SYNC_WORD  0xF0
#define LORA_FRQ        868E6
#define ACK_TIMEOUT     1000
#define RESEND_TIMEOUT  5000
#define IDLE_TIMEOUT    60000000
#define BROADCAST_CHAR  "\x01"
#define REGISTER_CHAR   "\x02"
#define ACK_CHAR        "\x03"
#define PING_CHAR       "\x04"
#define PONG_CHAR       "\x05"
#define SEPARATOR       "|"
#define BUZZER_FRQ      4500
#define BUZZER_DURATION 200

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

enum MsgStatus { idle, sending, delivered, fail };

struct Account {
  String user;
  bool online;
};

struct Message {
  String id;
  String from;
  String to;
  MsgStatus status;
  String text;
  unsigned long sentAt;
};


#endif //GLOBAL_H
