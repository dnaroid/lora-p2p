#include "global.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
Preferences preferences;

// message:  receiverIdx or -1|senderIdx|text

bool isBuzzing = false;
bool isSending = false;
unsigned long beepStartMillis = 0;
unsigned long sendStartMillis = 0;
int selectedUserIdx = OTHER_IDX;
int myUserIdx = MY_IDX;
std::vector<String> users;

void beep(const bool enable) {
  beepStartMillis = millis();
  isBuzzing = enable;
  if (enable) {
    tone(PIN_BUZZER, BUZZER_FRQ);
  } else {
    noTone(PIN_BUZZER);
  }
}

String encrypt(const String& message, const String& nickname) {
  String encryptedMessage = "";
  for (size_t i = 0; i < message.length(); i++) {
    const char encryptedChar = message[i] ^ nickname[i % nickname.length()];
    encryptedMessage += encryptedChar;
  }
  return encryptedMessage;
}

String decrypt(const String& encryptedMessage, const String& nickname) {
  return encrypt(encryptedMessage, nickname);
}

void sendRaw(const int fromIdx, const int toIdx, const String& text) {
  Serial.print("Sending from: ");
  Serial.print(fromIdx);
  Serial.print(" to: ");
  Serial.print(toIdx);
  Serial.print(" text: ");
  Serial.println(text);
  LoRa.beginPacket();
  LoRa.print(String(toIdx) + SEPARATOR + String(fromIdx) + SEPARATOR + text);
  LoRa.endPacket();
}


void logMessage(const String& message) {
  String currentLogs = preferences.getString(STORAGE_LOGS, "");
  currentLogs += message + "\n";
  preferences.putString(STORAGE_LOGS, currentLogs);
}

std::vector<String> splitString(const String& data, const char delimiter) {
  std::vector<String> result;
  int start = 0;
  int end;
  while ((end = data.indexOf(delimiter, start)) != -1) {
    result.push_back(data.substring(start, end));
    start = end + 1;
  }
  if (start < data.length()) result.push_back(data.substring(start));
  return result;
}

void receiveMessage() {
  String receivedText = "";
  while (LoRa.available()) receivedText += static_cast<char>(LoRa.read());
  LOG("[RAW]", receivedText);

  const int firstSeparatorIndex = receivedText.indexOf(SEPARATOR);
  const int secondSeparatorIndex = receivedText.indexOf(SEPARATOR, firstSeparatorIndex + 1);
  const int receiverIdx = receivedText.substring(0, firstSeparatorIndex).toInt();
  const int senderIdx = receivedText.substring(firstSeparatorIndex + 1, secondSeparatorIndex).toInt();
  const String message = receivedText.substring(secondSeparatorIndex + 1);

  if (receiverIdx == myUserIdx || receiverIdx == -1) {
    Serial.print("Received message from ");
    Serial.print(users[senderIdx]);
    Serial.print(": ");
    Serial.println(message);

    if (message == ACK_CHAR) {
      Serial.println("ACK received!");
      logMessage(message);
      isSending = false;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("delivered!");
      display.display();
      return;
    }
    if (message == PING_CHAR) {
      Serial.println("PING received!");
      sendRaw(myUserIdx, senderIdx,PONG_CHAR);
      Serial.println("PONG sent.");
      return;
    }
    if (message == PONG_CHAR) {
      Serial.println("PONG received!");
      return;
    }
    beep(true);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("From: " + users[senderIdx]);
    display.println(decrypt(message, users[myUserIdx]));
    display.display();
    logMessage(receivedText);
    sendRaw(myUserIdx, senderIdx, ACK_CHAR);
    Serial.println("ACK sent.");
  }
}

void sendMessage(const int receiverIdx, const String& text) {
  sendRaw(myUserIdx, receiverIdx, encrypt(text, users[receiverIdx]));
  sendStartMillis = millis();
  isSending = true;
  Serial.print("Sent to ");
  Serial.print(users[receiverIdx]);
  Serial.print(": ");
  Serial.println(text);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("sending to: " + users[receiverIdx]);
  display.println(text);
  display.display();
}

void setupDisplay() {
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextWrap(true);
  display.cp437(true);
  display.display();
}

void setupLoRa() {
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
  LoRa.setPins(PIN_LORA_CS, PIN_LORA_RST, PIN_LORA_IRQ);
  if (!LoRa.begin(LORA_FRQ)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }
  LoRa.setSyncWord(LORA_SYNC_WORD);
  Serial.println("LoRa initialized");
}

void setupStorage() {
  preferences.begin(STORAGE_SCOPE, false);
  const String storedUsers = preferences.getString(STORAGE_USERS, STORAGE_NICKS);
  users = splitString(storedUsers, ',');
  Serial.println("Stored Users: " + storedUsers);
  const String storedLogs = preferences.getString(STORAGE_LOGS, "");
  Serial.println("Stored Logs: " + storedLogs);
}

void setup() {
  START_SERIAL
  setupStorage();
  setupDisplay();
  setupLoRa();
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  display.print(users[myUserIdx]);
  display.println(" is ready!");
  display.display();
}

void loop() {
  const unsigned long currentMillis = millis();
  if (isBuzzing && (currentMillis - beepStartMillis >= BUZZER_DURATION)) beep(false);
  if (isSending && (currentMillis - sendStartMillis >= ACK_TIMEOUT)) {
    Serial.println("ACK not received, message may not have been delivered.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("not delivered!");
    display.display();
    isSending = false;
  }
  if (LoRa.parsePacket()) receiveMessage();
  if (Serial.available()) sendMessage(selectedUserIdx, Serial.readString());
  if (!isSending && digitalRead(PIN_BUTTON) == LOW) sendMessage(selectedUserIdx, "TEST");
}
