#include <Arduino.h>

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <esp_now.h>
#include <WiFi.h>

/* ------------------ Globals ------------------ */

static NimBLEHIDDevice* hid;
static NimBLECharacteristic* bootKeyboard;   // Report ID 0 (BOOT)
static NimBLECharacteristic* inputKeyboard;  // Report ID 1 (REPORT)

static bool connected = false;
static bool sentOnce = false;

int buttonOne = 0;
int buttonTwo = 21;
int buttonThree = 20;
int buttonFour = 10;

int buttonOneState = 0;
bool buttonOneInPress = false;

// Debounce variables for Button 1
int lastButtonOneReading = HIGH;
unsigned long lastDebounceTimeOne = 0;

int buttonTwoState = 0;
bool buttonTwoInPress = false;

// Debounce variables for Button 2
int lastButtonTwoReading = HIGH;
unsigned long lastDebounceTimeTwo = 0;

int buttonThreeState = 0;
bool buttonThreeInPress = false;

// Debounce variables for Button 3
int lastButtonThreeReading = HIGH;
unsigned long lastDebounceTimeThree = 0;

int buttonFourState = 0;
bool buttonFourInPress = false;

// Debounce variables for Button 4
int lastButtonFourReading = HIGH;
unsigned long lastDebounceTimeFour = 0;

const unsigned long debounceDelay = 100; // milliseconds

/* ------------------ Callbacks ------------------ */

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& c) override {
    connected = true;
    Serial.println(">>> CONNECTED <<<");
  }

  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& c, int reason) override {
    connected = false;
    sentOnce = false;
    Serial.printf(">>> DISCONNECTED (reason=%d) <<<\n", reason);
    NimBLEDevice::startAdvertising();
  }
};

/* ------------------ HID Report Map ------------------ */
/* Standard 6-key rollover keyboard, Report ID 1 */

static const uint8_t reportMap[] = {
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x06,  // Usage (Keyboard)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x01,  //   Report ID (1)
  0x05, 0x07,  //   Usage Page (Keyboard)
  0x19, 0xE0,  //   Usage Min (Left Control)
  0x29, 0xE7,  //   Usage Max (Right GUI)
  0x15, 0x00,
  0x25, 0x01,
  0x75, 0x01,
  0x95, 0x08,
  0x81, 0x02,  //   Input (Modifier byte)
  0x95, 0x01,
  0x75, 0x08,
  0x81, 0x01,  //   Input (Reserved)
  0x95, 0x06,
  0x75, 0x08,
  0x15, 0x00,
  0x25, 0x65,
  0x05, 0x07,
  0x19, 0x00,
  0x29, 0x65,
  0x81, 0x00,  //   Input (Key array)
  0xC0         // End Collection
};

/* ------------------ HID Keycodes ------------------ */

#define MOD_LSHIFT 0x02

#define KEY_H 0x0B
#define KEY_E 0x08
#define KEY_L 0x0F
#define KEY_O 0x12

#define KEY_1 0x1E
#define KEY_2 0x1F
#define KEY_3 0x20
#define KEY_4 0x21

#define KEY_ENTER 0x28

/* ------------------ Send Key (REPORT MODE ONLY) ------------------ */

void sendKey(uint8_t modifier, uint8_t keycode) {
  uint8_t report[8] = { modifier, 0x00, keycode, 0, 0, 0, 0, 0 };

  inputKeyboard->setValue(report, sizeof(report));
  inputKeyboard->notify();
  delay(10);

  uint8_t release[8] = { 0 };
  inputKeyboard->setValue(release, sizeof(release));
  inputKeyboard->notify();
}


enum class GameState {
  PRE_START,
  FIRST_NIGHT,
  DAY,
  PRE_REVEAL,
  NOMINATIONS_CONFIG,
  NOMINATIONS,
  POST_NOMINATIONS,
  KILL_PLAYER,
  REVIVE_PLAYER,
  NIGHT,
  END_GAME,
  OFF
};

GameState state = GameState::PRE_START;
GameState prevState = GameState::PRE_START;

int currentPlayerID = 0;


/* ------------------ OLED Display ------------------ */

#define PIN_SCK 4
#define PIN_MOSI 6
#define PIN_MISO 5  // not used by the OLED display but appears to be required for SPI.begin()
#define PIN_CS 7
#define PIN_DC 8
#define PIN_RST 9

Adafruit_SH1106G display(128, 64, &SPI, PIN_DC, PIN_RST, PIN_CS);

// Helper function to draw centered text for the controls
void drawCenteredText(const char* text, int x, int y, int w) {
  int16_t x1, y1;
  uint16_t tw, th;
  display.getTextBounds(text, 0, y, &x1, &y1, &tw, &th);
  int tx = x + (w - tw) / 2;
  display.setCursor(tx, y);
  display.print(text);
}

void updateStage() {
  display.fillRect(0, 0, 128, 16, SH110X_BLACK);
  display.setCursor(0, 0);
  
  if (state == GameState::KILL_PLAYER || state == GameState::REVIVE_PLAYER) {
    display.print("CMD: ");
  } else {
    display.print("Stage: ");
  }
  
  if (state == GameState::PRE_START) {
    display.print("Pre-Game");
  } else if (state == GameState::FIRST_NIGHT) {
    display.print("First Night");
  } else if (state == GameState::DAY) {
    display.print("Day");
  } else if (state == GameState::NOMINATIONS_CONFIG) {
    display.print("Nom Config");
  } else if (state == GameState::PRE_REVEAL) {
    display.print("Pre-Day Reveal");
  } else if (state == GameState::NOMINATIONS) {
    display.print("Nom Votes");
  } else if (state == GameState::POST_NOMINATIONS) {
    display.print("Post Nom");
  } else if (state == GameState::NIGHT) {
    display.print("Night");
  } else if (state == GameState::KILL_PLAYER) {
    display.print("Killing Player");
  } else if (state == GameState::REVIVE_PLAYER) {
    display.print("Reviving Player");
  } else if (state == GameState::END_GAME) {
    display.print("Game Over");
  } else if (state == GameState::OFF) {
    display.print("Off");
  }
  display.display();
}

void updatePlayer() {
  display.fillRect(0, 16, 128, 16, SH110X_BLACK);
  display.setCursor(0, 16);
  display.print("Current Player: ");
  display.print(currentPlayerID);
  display.display();
}

void updateOp(int index, const char* text) {
  if (index < 0 || index > 3) return;

  const int x = 0;
  const int y = 32 + (index * 8);
  const int w = 128;
  const int h = 8;

  display.fillRect(x, y, w, h, SH110X_BLACK);

  display.setCursor(x, y);
  display.print(index + 1);
  display.print(") ");
  display.print(text);

  display.display();
}

void updateOpsBulk(const char* op0, const char* op1, const char* op2, const char* op3) {
  updateOp(0, op0);
  updateOp(1, op1);
  updateOp(2, op2);
  updateOp(3, op3);
}

void updateOps() {
  if (state == GameState::PRE_START) {
    updateOpsBulk("Start", "N/A", "N/A", "N/A");
  } else if (state == GameState::FIRST_NIGHT) {
    updateOpsBulk("To Day", "N/A", "N/A", "N/A");
  } else if (state == GameState::DAY) {
    updateOpsBulk("Start Nominations", "Kill Player", "Revive Player", "To Night");
  } else if (state == GameState::PRE_REVEAL) {
    updateOpsBulk("Start Day", "N/A", "N/A", "N/A");
  } else if (state == GameState::NOMINATIONS_CONFIG) {
    updateOpsBulk("Next Player", "Previous Player", "Start Nominations", "Return");
  } else if (state == GameState::NOMINATIONS) {
    updateOpsBulk("Yes Vote", "No Vote", "Vote Skipped", "End");
  } else if (state == GameState::POST_NOMINATIONS) {
    updateOpsBulk("To Night", "Restart Nominations", "Kill Player", "End Game");
  } else if (state == GameState::NIGHT) {
    updateOpsBulk("To Pre-Day", "Kill Player", "Revive Player", "End Game");
  } else if (state == GameState::KILL_PLAYER) {
    updateOpsBulk("Next Player", "Previous Player", "Kill Current", "Return");
  } else if (state == GameState::REVIVE_PLAYER) {
    updateOpsBulk("Next Player", "Previous Player", "Revive Current", "Return");
  } else if (state == GameState::END_GAME) {
    updateOpsBulk("Go Back", "Good Wins", "Evil Wins", "Restart Game");
  } else if (state == GameState::OFF) {
    updateOpsBulk("Restart", "N/A", "N/A", "N/A");
  }
}

/* ------------------ ESP_NOW ------------------ */

char CMD_OFF[] = "off";

// Game-specific commands
char CMD_START[] = "start";
char CMD_DAY[] = "day";
char CMD_NIGHT[] = "night";
char CMD_PRE_REVEAL[] = "prerev";

char CMD_ALIVE[] = "alive";
char CMD_DEAD[] = "dead";
char CMD_NO_VOTE[] = "dvote";
char CMD_NOMINATIONS[] = "snomcon";
char CMD_SET_PLAYER[] = "splayer";
char CMD_START_KILL[] = "skill";
char CMD_END_KILL[] = "ekill";
char CMD_START_REVIVE[] = "srevive";
char CMD_END_REVIVE[] = "erevive";

char CMD_PREPARE_NOMINATIONS[] = "pnomin";
char CMD_START_NOMINATIONS[] = "snomin";
char CMD_POST_NOMINATIONS[] = "enomin";

char CMD_GAME_END[] = "endgame";

typedef struct botc_message {
  char command[32];
  float brightness = 100;
} botc_message;

// We will store received messages into "message"
botc_message message;

// Callback function that is called when data is received from ESP32-NOW
void onDataRecv(const esp_now_recv_info_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&message, incomingData, sizeof(message));

  Serial.println("Performing command check...");
  if (message.command == nullptr || message.command[0] == '\0') {
    Serial.println("Received null message.command!");
    return;
  }

  Serial.print("Running command: ");
  Serial.println(message.command);

  runCommand(message.command);

  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Command: ");
  if (!message.command) {
    Serial.println("ERROR UNKNOWN COMMAND");
  }
  else {
    Serial.println(message.command);
  }
}

/* HANDLER */
void runCommand(const char command[32]) {

  if (strcmp(command, CMD_START) == 0) {
    state = GameState::PRE_START;
  }

  if (strcmp(command, CMD_DAY) == 0) {
    state = GameState::DAY;
  }

  else if (strcmp(command, CMD_NIGHT) == 0) {
    if (state == GameState::PRE_START) {
      state = GameState::FIRST_NIGHT;
    } else {
      state = GameState::NIGHT;
    }
  }

  else if (strcmp(command, CMD_PRE_REVEAL) == 0) {
    state = GameState::PRE_REVEAL;
  }

  else if (strcmp(command, CMD_OFF) == 0) {
    state = GameState::OFF;
  }

  else if (strcmp(command, CMD_NOMINATIONS) == 0) {
    state = GameState::NOMINATIONS_CONFIG;
  }

  else if (strcmp(command, CMD_START_NOMINATIONS) == 0) {
    state = GameState::NOMINATIONS;
  }

  else if (strcmp(command, CMD_POST_NOMINATIONS) == 0) {
    state = GameState::POST_NOMINATIONS;
  }

  else if (strcmp(command, CMD_START_KILL) == 0) {
    prevState = state;
    state = GameState::KILL_PLAYER;
  } 

  else if (strcmp(command, CMD_END_KILL) == 0) {
    state = prevState;
  }

  else if (strcmp(command, CMD_START_REVIVE) == 0) {
    prevState = state;
    state = GameState::REVIVE_PLAYER;
  }

  else if (strcmp(command, CMD_END_REVIVE) == 0) {
    state = prevState;
  }

  else if (strcmp(command, CMD_GAME_END) == 0) {
    state = GameState::END_GAME;
  }

  else if (strncmp(command, CMD_SET_PLAYER, 7) == 0) { // hard-coded to "splayer"
    currentPlayerID = getSplitID(command);
    updatePlayer();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
  }

  updateStage();

  updateOps();
}


/* ------------------ Setup ------------------ */

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\nBOOT");
  Serial.println("Enabling BLE...");

  /* ---- BLE Init ---- */
  NimBLEDevice::init("ESP32 Keyboard");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  /* ---- Security (Android-safe) ---- */
  NimBLEDevice::setSecurityAuth(true, false, true);  // bonding YES, MITM NO
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  /* ---- Server ---- */
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  Serial.println("Server callbacks set");

  /* ---- HID Device ---- */
  hid = new NimBLEHIDDevice(server);

  // REQUIRED so Android accepts us as a keyboard
  bootKeyboard = hid->getInputReport(0);   // BOOT keyboard
  inputKeyboard = hid->getInputReport(1);  // Report keyboard

  hid->setManufacturer("ESP32-C3");
  hid->setHidInfo(0x00, 0x01);
  hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->setBatteryLevel(100);

  hid->startServices();

  /* ---- Advertising ---- */
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(HID_KEYBOARD);
  adv->addServiceUUID((uint16_t)0x1812);  // HID
  adv->addServiceUUID((uint16_t)0x180F);  // Battery
  adv->start();

  Serial.println("Advertising HID keyboard...");

  Serial.println("Enabling SPI OLED");

  // OLED display - start SPI
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

  // Waits for OLED display to reset / turn on
  // First parameter is unused, second one causes OLED display to reset when board resets
  if (!display.begin(0, true)) {
    while (true) { delay(1000); }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // Setup default OLED display
  updateStage();
  updatePlayer();
  updateOps();

  Serial.println("Enabling Wifi...");
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Print MAC address to serial
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

  // ESP-NOW initialised; setup receiver callback
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("WiFi Setup Done.");

  // And finally, setup our button pins
  pinMode(buttonOne, INPUT_PULLUP);
  pinMode(buttonTwo, INPUT_PULLUP);
  pinMode(buttonThree, INPUT_PULLUP);
  pinMode(buttonFour, INPUT_PULLUP);

  // Initialize debounce state: read initial stable states and align "last reading" vars
  buttonOneState = digitalRead(buttonOne);
  lastButtonOneReading = buttonOneState;
  lastDebounceTimeOne = 0;

  buttonTwoState = digitalRead(buttonTwo);
  lastButtonTwoReading = buttonTwoState;
  lastDebounceTimeTwo = 0;

  buttonThreeState = digitalRead(buttonThree);
  lastButtonThreeReading = buttonThreeState;
  lastDebounceTimeThree = 0;

  buttonFourState = digitalRead(buttonFour);
  lastButtonFourReading = buttonFourState;
  lastDebounceTimeFour = 0;

  Serial.println("Setup complete!");
}


/* ------------------ Loop ------------------ */

void loop() {
  // Button 1 debounce
  int reading1 = digitalRead(buttonOne);
  if (reading1 != lastButtonOneReading) {
    lastDebounceTimeOne = millis();
  }
  if ((millis() - lastDebounceTimeOne) > debounceDelay) {
    if (reading1 != buttonOneState) {
      buttonOneState = reading1;
      if (buttonOneState == LOW && !buttonOneInPress) {
        Serial.println("Button pressed!");
        sendKey(0, KEY_1);
        sendKey(0, KEY_ENTER);
        Serial.println("Key 1 sent!");
        buttonOneInPress = true;
      } else if (buttonOneState == HIGH && buttonOneInPress) {
        buttonOneInPress = false;
        Serial.println("Button released!");
      }
    }
  }
  lastButtonOneReading = reading1;

  // Button 2 debounce
  int reading2 = digitalRead(buttonTwo);
  if (reading2 != lastButtonTwoReading) {
    lastDebounceTimeTwo = millis();
  }
  if ((millis() - lastDebounceTimeTwo) > debounceDelay) {
    if (reading2 != buttonTwoState) {
      buttonTwoState = reading2;
      if (buttonTwoState == LOW && !buttonTwoInPress) {
        Serial.println("Button pressed!");
        sendKey(0, KEY_2);
        sendKey(0, KEY_ENTER);
        Serial.println("Key 2 sent!");
        buttonTwoInPress = true;
      } else if (buttonTwoState == HIGH && buttonTwoInPress) {
        buttonTwoInPress = false;
        Serial.println("Button released!");
      }
    }
  }
  lastButtonTwoReading = reading2;

  // Button 3 debounce
  int reading3 = digitalRead(buttonThree);
  if (reading3 != lastButtonThreeReading) {
    lastDebounceTimeThree = millis();
  }
  if ((millis() - lastDebounceTimeThree) > debounceDelay) {
    if (reading3 != buttonThreeState) {
      buttonThreeState = reading3;
      if (buttonThreeState == LOW && !buttonThreeInPress) {
        Serial.println("Button pressed!");
        sendKey(0, KEY_3);
        sendKey(0, KEY_ENTER);
        Serial.println("Key 3 sent!");
        buttonThreeInPress = true;
      } else if (buttonThreeState == HIGH && buttonThreeInPress) {
        buttonThreeInPress = false;
        Serial.println("Button released!");
      }
    }
  }
  lastButtonThreeReading = reading3;

  // Button 4 debounce
  int reading4 = digitalRead(buttonFour);
  if (reading4 != lastButtonFourReading) {
    lastDebounceTimeFour = millis();
  }
  if ((millis() - lastDebounceTimeFour) > debounceDelay) {
    if (reading4 != buttonFourState) {
      buttonFourState = reading4;
      if (buttonFourState == LOW && !buttonFourInPress) {
        Serial.println("Button pressed!");
        sendKey(0, KEY_4);
        sendKey(0, KEY_ENTER);
        Serial.println("Key 4 sent!");
        buttonFourInPress = true;
      } else if (buttonFourState == HIGH && buttonFourInPress) {
        buttonFourInPress = false;
        Serial.println("Button released!");
      }
    }
  }
  lastButtonFourReading = reading4;
}

/* ---- Util ---- */
int getSplitID(const char command[32]) {
  /* Gets an ID from a command in the form "command,id" */
  return atoi(strchr(command, ',') + 1);
}
