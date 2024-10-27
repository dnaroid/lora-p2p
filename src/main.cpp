#include "global.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include "driver/rtc_io.h"
#include <LittleFS.h>

#ifndef PIN_BUTTON
#include <keyboard.h>
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

bool isBuzzing = false;
bool isWakeMsg = false;
unsigned long beepStartMillis = 0;
unsigned long sendStartMillis = 0;
unsigned long idleMillis = 0;
std::vector<Account> users;
std::vector<Message> messages;

#ifdef PIN_BUTTON
String myName = "Bar";
String targetName = "Foo";
#else
String myName = "Foo";
String targetName = "Bar";
#endif

String buff = "";
volatile bool isIncomeMsg = false;

void setupFileSystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
}

void saveMessagesToFile(const std::vector<Message>& messages, const char* filename) {
  if (!LittleFS.exists(filename)) {
    Serial.println("File does not exist. Creating file...");
  }

  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file!");
    return;
  }

  for (const auto& msg : messages) {
    String line = msg.id + "," + msg.from + "," + msg.to + "," + String(msg.status) + "," + msg.text + SEPARATOR + String(msg.sentAt) + "\n";
    file.print(line);
  }
  file.close();
  Serial.println("Messages saved to file.");
}

void loadMessagesFromFile(std::vector<Message>& messages, const char* filename) {
  if (!LittleFS.exists(filename)) {
    Serial.println("File not found!");
    return;
  }

  File file = LittleFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading!");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    Message msg;
    sscanf(line.c_str(), "%[^,],%[^,],%[^,],%d,%[^,],%lu", msg.id, msg.from, msg.to, &msg.status, msg.text, &msg.sentAt);
    messages.push_back(msg);
    Serial.print((msg.to == BROADCAST_CHAR ? "ALL" : msg.to));
    Serial.print("<" + msg.from);
    Serial.print(":");
    Serial.println(msg.text);
  }
  file.close();
  Serial.println("Messages loaded from file.");
}

void onReceive(const int packetSize) {
  if (packetSize == 0) return;
  isIncomeMsg = true;
}

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

void sendRaw(const String& toName, const String& id, const String& text, const bool encrypted = false) {
  LoRa.beginPacket();
  LoRa.print(toName);
  LoRa.print(SEPARATOR);
  LoRa.print(myName);
  LoRa.print(SEPARATOR);
  LoRa.print(id);
  LoRa.print(SEPARATOR);
  LoRa.print(encrypted ? encrypt(text, toName) : text);
  LoRa.endPacket(true);
  LoRa.receive();
  Serial.print(myName);
  Serial.print(">" + (toName == BROADCAST_CHAR ? "ALL" : toName));
  Serial.print(":");
  Serial.println(text == PING_CHAR ? "PING" : text == PONG_CHAR ? "PONG" : text == ACK_CHAR ? "ACK" : text);
  if (encrypted) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("to " + (toName == BROADCAST_CHAR ? "ALL" : toName));
    display.print(": ");
    display.println(text);
    display.display();
  }
}

void createMessage(const String& receiverName, const String& text) {
  messages.emplace_back(Message{
    .id = static_cast<String>(random()),
    .from = myName,
    .to = receiverName,
    .status = idle,
    .text = text,
    .sentAt = 0,
  });
}

void receiveMessage() {
  String receivedText = "";
  while (LoRa.available()) receivedText += static_cast<char>(LoRa.read());

  const int firstSeparatorIndex = receivedText.indexOf(SEPARATOR);
  const int secondSeparatorIndex = receivedText.indexOf(SEPARATOR, firstSeparatorIndex + 1);
  const int thirdSeparatorIndex = receivedText.indexOf(SEPARATOR, secondSeparatorIndex + 1);
  const String receiverName = receivedText.substring(0, firstSeparatorIndex);
  const String senderName = receivedText.substring(firstSeparatorIndex + 1, secondSeparatorIndex);
  const String id = receivedText.substring(secondSeparatorIndex + 1, thirdSeparatorIndex);
  const String payload = receivedText.substring(thirdSeparatorIndex + 1);

  Serial.print((receiverName == BROADCAST_CHAR ? "ALL" : receiverName));
  Serial.print("<" + senderName);
  Serial.print(":");
  Serial.println(payload == PING_CHAR ? "PING" : payload == PONG_CHAR ? "PONG" : payload == ACK_CHAR ? "ACK" : decrypt(payload, receiverName));

  // todo process all messages as net
  if (receiverName == myName || receiverName == BROADCAST_CHAR) {
    if (payload == ACK_CHAR) {
      for (auto& m : messages) {
        if (m.id == id) {
          m.status = delivered;
          m.sentAt = millis();
          display.println("delivered!");
          display.display();
          break;
        }
      }
    } else if (payload == PING_CHAR) {
      sendRaw(senderName, "",PONG_CHAR);
      display.println("online: " + senderName);
      display.display();
      for (auto& m : messages) {
        if (m.to == senderName && m.status == fail) m.status = idle;
      }
    } else if (payload == PONG_CHAR) {
      for (auto& m : messages) {
        if (m.to == senderName && m.status == fail) m.status = idle;
      }
      display.println("online: " + senderName);
      display.display();
    } else { // new message
      sendRaw(senderName, id, ACK_CHAR);
      beep(true);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(senderName);
      display.print(": ");
      display.println(decrypt(payload, myName));
      display.display();
      idleMillis = millis();
    }
  }
  isIncomeMsg = false;
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
  display.fillScreen(SSD1306_WHITE);
  display.display();
  delay(100);
  display.clearDisplay();
  display.display();
}

void setupLoRa() {
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
  LoRa.setPins(PIN_LORA_CS, -1, PIN_LORA_IRQ);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  // pinMode(PIN_LORA_IRQ, INPUT_PULLDOWN);
  // isWakeMsg = digitalRead(PIN_LORA_IRQ);
  if (!LoRa.begin(LORA_FRQ)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }
  // LoRa.setSpreadingFactor(12);
}

void goSleep() {
  LOG("--sleep--");
  // saveMessagesToFile(messages, MESSAGES_FILE);
  display.ssd1306_command(SSD1306_CHARGEPUMP); //into charger pump set mode
  display.ssd1306_command(0x10); //turn off charger pump
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  rtc_gpio_pullup_dis(GPIO_NUM_5);
  rtc_gpio_pulldown_en(GPIO_NUM_5);
  rtc_gpio_pullup_dis(GPIO_NUM_0); // ENTER btn
  rtc_gpio_pulldown_en(GPIO_NUM_0);

  esp_deep_sleep_enable_gpio_wakeup(IRQ_PIN_BITMASK(GPIO_NUM_5) | IRQ_PIN_BITMASK(GPIO_NUM_0), ESP_GPIO_WAKEUP_GPIO_HIGH);
  esp_deep_sleep_start();
}

void updateMessages(const unsigned long curMillis) {
  for (auto& m : messages) {
    if (m.status == idle) {
      m.status = sending;
      m.sentAt = millis();
      sendRaw(m.to, m.id, m.text, true);
      idleMillis = curMillis;
    } else if (m.status == sending) {
      if (curMillis - m.sentAt >= ACK_TIMEOUT) {
        m.sentAt = 0;
        m.status = fail;
        m.sentAt = curMillis;
        display.println("not delivered!");
        display.display();
        idleMillis = curMillis;
      }
    } else if (m.status == fail) {
      if (curMillis - m.sentAt >= RESEND_TIMEOUT + random(500)) {
        m.sentAt = 0;
        m.status = idle;
        display.display();
        idleMillis = curMillis;
      }
    }
  }
}

void setup() {
  START_SERIAL
  setupLoRa();
  setupDisplay();
  // setupFileSystem();
  // loadMessagesFromFile(messages, MESSAGES_FILE);
  pinMode(PIN_BUZZER, OUTPUT);
#ifdef PIN_BUTTON
  pinMode(PIN_BUTTON, INPUT_PULLUP);
#else
  setupKeyboard();
#endif
  display.print(myName);
  display.println(" ready");
  display.display();
  LoRa.onReceive(onReceive);
  LoRa.receive();
  idleMillis = millis();
  sendRaw(BROADCAST_CHAR, "",PING_CHAR);
}

void loop() {
  const unsigned long currentMillis = millis();
  if (isIncomeMsg) receiveMessage();
  updateMessages(currentMillis);
  if (isBuzzing && (currentMillis - beepStartMillis >= BUZZER_DURATION)) beep(false);
  if (currentMillis > idleMillis + IDLE_TIMEOUT) goSleep();
  if (Serial.available()) createMessage(targetName, Serial.readString());
#ifdef PIN_BUTTON
  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(50);
    if (digitalRead(PIN_BUTTON) == LOW) {
      idleMillis = currentMillis;
      buff = "TEST_MSG";
      createMessage(targetName, buff);
    }
  }
#else
  char key = getKeyPressed();
  if (key) {
    idleMillis = currentMillis;
    Serial.print(key);
    if (key == 'Q') goSleep();
    if (key == '\x0d') {
      createMessage(targetName, buff);
      buff = "";
    } else {
      buff += key;
      display.print(key);
      display.display();
    }
  }
#endif
}
