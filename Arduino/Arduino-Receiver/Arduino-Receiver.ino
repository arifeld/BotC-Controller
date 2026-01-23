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

char CMD_SET_NOMINATED[] = "snomin";
char CMD_PREPARE_FOR_NOMINATION[] = "pnomin";
char CMD_VOTED_YES[] = "vyes";
char CMD_VOTED_NO[] = "vno";
char CMD_VOTED_SKIPPED[] = "vskip";

char CMD_GOOD_WINS[] = "goodwins";
char CMD_EVIL_WINS[] = "evilwins";

int redPin = 3;
int greenPin = 2;
int bluePin = 1;

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
  SPECIAL,
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
    specialColour(255, 0, 0, brightness);
  }

  else if (strcmp(CMD_BLUE, command) == 0) {
    Serial.println("CMD: Blue");
    specialColour(0, 0, 255, brightness);
  }

  else if (strcmp(CMD_GREEN, command) == 0) {
    Serial.println("CMD: Green");
    specialColour(0, 255, 0, brightness);
  }

  else if (strcmp(CMD_GOOD_WINS, command) == 0) {
    Serial.println("CMD: GOOD WINS");
    specialColour(0, 255, 0, brightness);
  }

  else if (strcmp(CMD_EVIL_WINS, command) == 0) {
    Serial.println("CMD: EVIL WINS");
    specialColour(255, 0, 0, brightness);
  }

  else if (strcmp(CMD_PREPARE_FOR_NOMINATION, command) == 0) {
    state = GameState::DAY;
  }

  else if (strcmp(CMD_SET_NOMINATED, command) == 0) {
    specialColour(139, 0, 0, brightness);
  }

  else if (strcmp(CMD_VOTED_YES, command) == 0) {
    if (playerState == PlayerState::DEAD_ONE_VOTE) {
      playerState = PlayerState::DEAD;
    }
    specialColour(0, 255, 0, brightness);
  }

  else if (strcmp(CMD_VOTED_NO, command) == 0) {
    specialColour(255, 0, 0, brightness);
  }

  else if (strcmp(CMD_VOTED_SKIPPED, command) == 0) {
    specialColour(0, 0, 0, brightness);
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

  turnOff();

  playerState = PlayerState::ALIVE;

  Serial.println("Good to go!");
}

void loop() {
  if (state == GameState::DAY) {
    flicker(100);
  } else if (state == GameState::NIGHT) {
    turnOff();
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
  } else {
    turnOff();
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

void specialColour(int r, int g, int b, float brightness) {
  state = GameState::SPECIAL;
  setColour(r, g, b, brightness);
}
