/*
  BOTC-Controller Arduino Receiver

  Controls a connected RGB LED based on messages received from the sender.

  Uses an ESP32-C3 board in station mode, allowing us to access it without requiring connecting to WiFi.

  Pins (using a common cathode RGB):
  - Pin 1: R
  - Pin 2: G
  - Pin 3: B

*/


#include <esp_now.h>
#include <WiFi.h>

// Configuration / general commands
char CMD_RED[] = "red";
char CMD_BLUE[] = "blue";
char CMD_GREEN[] = "green";
char CMD_OFF[] = "off";

// Game-specific commands
char CMD_START[] = "start";
char CMD_DAY[] = "day";
char CMD_NIGHT[] = "night";
char CMD_ALIVE[] = "alive";
char CMD_DEAD[] = "dead";
char CMD_NO_VOTE[] = "dvote";

int redPin = 1;
int greenPin = 2;
int bluePin = 3;

typedef struct botc_message {
  char command[32];
  float brightness = 100;
} botc_message;

// We will store received messages into "message"
botc_message message;

// Enum to store the state of the player
enum class PlayerState {
  ALIVE,
  DEAD_ONE_VOTE,
  DEAD
};

PlayerState playerState;

enum class GameState {
  DAY,
  NIGHT,
  OFF
};

GameState state;

void runCommand(const char command[32], const float brightness = 100) {

  if (strcmp(command, CMD_START) == 0) {
    playerState = PlayerState::ALIVE;
    startNight();
  }

  else if (strcmp(command, CMD_DAY) == 0) {
    startDay();
  }

  else if (strcmp(command, CMD_NIGHT) == 0) {
    startNight();
  }

  else if (strcmp(command, CMD_DEAD) == 0) {
    onPlayerDeath();
  }

  else if (strcmp(command, CMD_NO_VOTE) == 0) {
    onDeadVoteUsed();
  }

  else if (strcmp(command, CMD_ALIVE) == 0) {
    onPlayerAlive();
  }

  else if (strcmp(CMD_RED, command) == 0) {
    Serial.println("CMD: Red");
    setColour(255, 0, 0, brightness);
  }

  else if (strcmp(CMD_BLUE, command) == 0) {
    Serial.println("CMD: Blue");
    setColour(0, 0, 255, brightness);
  }

  else if (strcmp(CMD_GREEN, command) == 0) {
    Serial.println("CMD: Green");
    setColour(0, 255, 0, brightness);
  }

  else if (strcmp(CMD_OFF, command) == 0) {
    Serial.println("CMD: Off");
    setColour(0, 0, 0, 100);
    state = GameState::OFF;
  }
}

// Callback function that is called when data is received from ESP32-NOW
void onDataRecv(const esp_now_recv_info_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&message, incomingData, sizeof(message));
  runCommand(message.command, message.brightness);

  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Command: ");
  Serial.println(message.command);
  Serial.print("Brightness: ");
  Serial.println(message.brightness);
  Serial.println();
}

void setup() {

  // Init serial monitor
  Serial.begin(115200);

  // Set the ESP32 into WiFi station mode
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

  // LED setup
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  playerState = PlayerState::ALIVE;
}


void loop() {
  if (state == GameState::DAY) {
    flicker(100);
  } else if (state == GameState::NIGHT) {
    flicker(50);
  }
}

void flicker(int brightness) {
  if (playerState == PlayerState::ALIVE) {
    setColour(random(120) + 135, random(10) + 30, random(2), 50);
    delay(100);
  } else if (playerState == PlayerState::DEAD_ONE_VOTE) {
    setColour(random(120) + 135, random(10) + 30, random(2), 50);
    delay(1000);
    turnOff();
    delay(1000);
  }
}

void setColour(int r, int g, int b, float brightness) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

void turnOff() {
  setColour(0, 0, 0, 100);
}

void startDay() {
  state = GameState::DAY;

  if (playerState == PlayerState::ALIVE) {
    setColour(255, 210, 28, 100);  // candlelight neon
  }
}

void startNight() {
  state = GameState::NIGHT;
}

void onPlayerDeath() {
  playerState = PlayerState::DEAD_ONE_VOTE;
}

void onDeadVoteUsed() {
  playerState = PlayerState::DEAD;
}

void onPlayerAlive() {
  playerState = PlayerState::ALIVE;
}
