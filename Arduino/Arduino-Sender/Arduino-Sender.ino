#include <esp_now.h>
#include <WiFi.h>

// MAC addresses of all ESP32 boards to use
uint8_t macAddresses[][6] = {
  { 0xAC, 0xA7, 0x04, 0xB9, 0x74, 0x60 },
  { 0x80, 0xF1, 0xB2, 0x88, 0xBB, 0x98 },
  { 0xAC, 0xA7, 0x04, 0xBA, 0x67, 0x6C },
  { 0xAC, 0xA7, 0x04, 0xB8, 0x79, 0x94 },
  { 0xAC, 0xA7, 0x04, 0xBA, 0x92, 0xD8 },
  { 0x80, 0xF1, 0xB2, 0x6D, 0xC0, 0x2C },
  { 0xAC, 0xA7, 0x04, 0xBA, 0xA6, 0x7C },
  { 0x80, 0xF1, 0xB2, 0x8A, 0xFE, 0x1C },
  { 0xAC, 0xA7, 0x04, 0xB9, 0xB8, 0xA0 },
  { 0xAC, 0xA7, 0x04, 0xBA, 0xAA, 0x40 },
  { 0xAC, 0xA7, 0x04, 0xBA, 0x96, 0xB0 },
  { 0xAC, 0xA7, 0x04, 0xB8, 0x3E, 0x98 },
  { 0xAC, 0xA7, 0x04, 0xBA, 0xB2, 0xCC },
  { 0xAC, 0xA7, 0x04, 0xB8, 0x7C, 0x3C },
  { 0x80, 0xF1, 0xB2, 0x88, 0xBD, 0x84 },
};

uint8_t controllerMAC[6] = {
  0xAC, 0xA7, 0x04, 0xBA, 0x40, 0xF8
};

const int totalDevices = sizeof(macAddresses) / sizeof(macAddresses[0]);
int totalPlayers = totalDevices;  // by default we assume all devices are in play

// Basic lookup table. Each index maps to a MAC address in the above array.
// This way (since we will have a maximum of 15 players/devices), we can dynamically set which player is using which device/MAC.
int playerDevice[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

const String SERIAL_RED = String("red");

const String SERIAL_BLUE = String("blue");

const String SERIAL_GREEN = String("green");

const String SERIAL_START = String("start");

const String SERIAL_DAY = String("day");

const String SERIAL_NOMINATIONS = String("noms");
const String SERIAL_NEXT_PLAYER = String("nplayer");
const String SERIAL_PREV_PLAYER = String("pplayer");
const String SERIAL_SET_PLAYER = String("splayer");

const String SERIAL_START_KILL = String("skill");
const String SERIAL_END_KILL = String("ekill");

const String SERIAL_START_REVIVE = String("srevive");
const String SERIAL_END_REVIVE = String("erevive");

const String SERIAL_NIGHT = String("night");

const String SERIAL_OFF = String("off");

const String SERIAL_START_CONFIG = String("sconfig");
const String SERIAL_NEXT_CONFIG_DEVICE = String("ndevice");
const String SERIAL_SET_CONFIG_DEVICE = String("sdevice");
const String SERIAL_END_CONFIG = String("econfig");

const String SERIAL_PLAYER_DEAD = String("dead");
const String SERIAL_PLAYER_FORCE_DEAD = String("forced");  // command is in form "forced,id" where id is the player ID

const String SERIAL_PLAYER_NO_VOTE = String("dvote");

const String SERIAL_PLAYER_REVIVE = String("revive");
const String SERIAL_PLAYER_FORCE_REVIVE = String("forcer"); // command is in form "forcer,id" where id is the player ID

const String SERIAL_START_NOMINATION_CONFIG = String("snomcon");
const String SERIAL_PREPARE_FOR_NOMINATIONS = String("pnomin");
const String SERIAL_START_NOMINATIONS = String("snomin");
const String SERIAL_POST_NOMINATIONS = String("enomin");
const String SERIAL_VOTED_YES = String("vyes");
const String SERIAL_VOTED_NO = String("vno");
const String SERIAL_VOTE_SKIPPED = String("vskip");

const String SERIAL_END_GAME = String("endgame");
const String SERIAL_GOOD_WINS = String("goodwins");
const String SERIAL_EVIL_WINS = String("evilwins");

const String DEBUG_CURRENT_PLAYER = String("cur");


char serialCharCommand[32];

typedef struct botc_message {
  char command[32];
  float brightness = 100;
} botc_message;


// We will store received messages into "message"
botc_message message;

// The player we will run a command on
int playerID = -1;

int currentPlayerID = 0;

String serialString;

esp_now_peer_info_t peerInfo;


/*
  Configuration related variables
*/
enum class SenderState {
  CONFIGURATION,
  READY
};

SenderState senderState;

int configPlayer = 0;  // Which player we are setting
int configDevice = 0;  // Which device we are up to

// callback when data is sent
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Last Packet Delivery Fail");
  }
}

/*
  Function declarations
*/
String getSplitValue(String data);


void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // On send callback registration
  esp_now_register_send_cb(OnDataSent);

  // Register all peers
  for (int i = 0; i < totalDevices; i++) {
    memcpy(peerInfo.peer_addr, macAddresses[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.print("Failed to add peer ");
      Serial.println(i);
    } else {
      Serial.print("Successfully added peer ");
      Serial.println(i);
    }
  }

  // Add the controller peer
  memcpy(peerInfo.peer_addr, controllerMAC, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add controller peer.");
  } else {
    Serial.println("Successfully added controller peer.");
  }
}


// Define our commands based on what action we take, this helps us to reduce code repetition

String broadcastCommands[] = { SERIAL_RED, SERIAL_BLUE, SERIAL_START, SERIAL_DAY, SERIAL_NIGHT, SERIAL_GOOD_WINS, SERIAL_EVIL_WINS };
const int broadcastCommandCount = sizeof(broadcastCommands) / sizeof(broadcastCommands[0]);

String controllerCommands[] = { SERIAL_NOMINATIONS, SERIAL_START_KILL, SERIAL_END_KILL, SERIAL_START_REVIVE, SERIAL_END_REVIVE, SERIAL_START_NOMINATION_CONFIG, SERIAL_POST_NOMINATIONS, SERIAL_END_GAME };
const int controllerCommandCount = sizeof(controllerCommands) / sizeof(controllerCommands[0]);


void loop() {

  while (Serial.available()) {
    serialString = Serial.readString();
    serialString.trim();
  }

  if (serialString.length() > 0) {
    Serial.println(serialString);

    // Broadcast to all commands
    if (stringInArray(serialString, broadcastCommands, broadcastCommandCount)) {
      broadcast(serialString);

    // Send to controller only commands
    } else if (stringInArray(serialString, controllerCommands, controllerCommandCount)) {
      sendController(serialString);
    // More complex commands
    } else if (serialString == SERIAL_NEXT_PLAYER) {
      nextPlayer();
    } else if (serialString == SERIAL_PREV_PLAYER) {
      previousPlayer();
    } else if (serialString == SERIAL_START_CONFIG) {
      startConfiguration();
    } else if (serialString == SERIAL_NEXT_CONFIG_DEVICE) {
      configNextDevice();
    } else if (serialString == SERIAL_SET_CONFIG_DEVICE) {
      configSetDeviceForPlayer();
    } else if (serialString == SERIAL_END_CONFIG) {
      endConfiguration();
    } else if (serialString == SERIAL_PLAYER_DEAD) {
      sendCommand(SERIAL_PLAYER_DEAD, getPlayerDevice(currentPlayerID));
    } else if (serialString == SERIAL_PLAYER_REVIVE) {
      sendCommand(SERIAL_PLAYER_REVIVE, getPlayerDevice(currentPlayerID));
    } else if (serialString == SERIAL_START_NOMINATIONS) {
      startNominationsCurrentPlayer();
    } else if (serialString == SERIAL_VOTED_YES) {
      currentPlayerVotedYes();
    } else if (serialString == SERIAL_VOTED_NO) {
      currentPlayerVotedNo();
    } else if (serialString == SERIAL_VOTE_SKIPPED) {
      currentPlayerVoteSkipped();
    } else if (serialString.startsWith(SERIAL_PLAYER_FORCE_DEAD)) {
      // Get the player ID in range [0, 14]
      playerID = getSafePlayerID(serialString);

      Serial.print("Force dead player: ");
      Serial.println(playerID);

      sendCommand(SERIAL_PLAYER_DEAD, getPlayerDevice(playerID));

    } else if (serialString.startsWith(SERIAL_PLAYER_NO_VOTE)) {
      // Get the player ID in range [0, 14]
      playerID = getSafePlayerID(serialString);

      Serial.print("Dead vote used for player: ");
      Serial.println(playerID);

      sendCommand(SERIAL_PLAYER_NO_VOTE, getPlayerDevice(playerID));
    } else if (serialString.startsWith(SERIAL_PLAYER_FORCE_REVIVE)) {
      playerID = getSafePlayerID(serialString);

      Serial.print("Force revive player: ");
      Serial.println(playerID);

      sendCommand(SERIAL_PLAYER_REVIVE, getPlayerDevice(playerID));
    } else {
      Serial.print("Unknown command: ");
      Serial.print(serialString);
      Serial.println();
    }

    serialString = "";
  }
}
void sendCommand(String command, uint8_t *peer) {
  /* Copies the string command into the botc_message struct, then sends to the specified peer. */
  command.toCharArray(serialCharCommand, command.length() + 1);  // unsafe but we're ignoring that for the purposes of this application

  strcpy(message.command, serialCharCommand);
  sendMessage(message, peer);
}

void sendController(String command) {
  command.toCharArray(serialCharCommand, command.length() + 1);

  strcpy(message.command, serialCharCommand);
  sendMessage(message, controllerMAC);
}

void broadcast(String command) {
  /* Utility function to broadcast a command to all peers. */
  sendCommand(command, 0);  // Sending NULL / 0 as the peer_addr broadcasts to all connected peers
}

void sendMessage(const botc_message message, const uint8_t *peer_addr) {
  /* Sends a message to the specified peer MAC address. Set peer_addr to NULL/0 to broadcast. */
  esp_err_t result = esp_now_send(peer_addr, (uint8_t *)&message, sizeof(message));
  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

/* ------ COMMAND FUNCTIONS ------ */

void nextPlayer() {
  currentPlayerID = modulo((currentPlayerID + 1), totalPlayers);
  sendController(SERIAL_SET_PLAYER + "," + String(currentPlayerID));
  Serial.print("Current player: ");
  Serial.println(currentPlayerID);
}

void previousPlayer() {
  currentPlayerID = modulo((currentPlayerID - 1), totalPlayers);
  sendController(SERIAL_SET_PLAYER + "," + String(currentPlayerID));
  Serial.print("Current player: ");
  Serial.println(currentPlayerID);
}

void startNominationsCurrentPlayer() {
  // Turn on all the lights to day mode if they aren't already (e.g. if we are on a second nomination)
  broadcast(SERIAL_PREPARE_FOR_NOMINATIONS);
  delay(50); // make sure the nominated player also receives the message before sending the next command
  sendController(SERIAL_START_NOMINATIONS);
  sendCommand(SERIAL_START_NOMINATIONS, getPlayerDevice(currentPlayerID));
  
  // Automatically move to the next player as they will have the first vote
  delay(50);
  nextPlayer();
}

void currentPlayerVotedYes() {
  sendCommand(SERIAL_VOTED_YES, getPlayerDevice(currentPlayerID));
  nextPlayer();
}

void currentPlayerVotedNo() {
  sendCommand(SERIAL_VOTED_NO, getPlayerDevice(currentPlayerID));
  nextPlayer();
}

void currentPlayerVoteSkipped() {
  sendCommand(SERIAL_VOTE_SKIPPED, getPlayerDevice(currentPlayerID));
  nextPlayer();
}

void startConfiguration() {
  Serial.println("Starting configuration with Player ID 0 and Device ID 0");

  /* Start device configuration, resetting any prior configs. */
  for (int i = 0; i < totalDevices; i++) {
    playerDevice[i] = -1;
  }
  senderState = SenderState::CONFIGURATION;

  /* Broadcast all to turn off. */
  broadcast(SERIAL_OFF);

  configPlayer = 0;
  configDevice = 0;

  // Reset how many players
  totalPlayers = 0;

  attemptConfigDevice(configDevice);
}

void attemptEndConfigDevice(int id) {
  /* Sends a turn-off command to id-th device. */
  sendCommand(SERIAL_OFF, macAddresses[id]);
}

void attemptConfigDevice(int id) {
  /* Attempts to set the colour of the id-th device to blue, to indicate it is the current device being configured. */
  sendCommand(SERIAL_BLUE, macAddresses[id]);
}

void configNextDevice() {
  /* Move to the next device in configuration, bounding between 0-14 inclusive. */
  Serial.print("CONFIG: Skipping device ");
  Serial.println(configDevice);
  // Turn off the light for the old device
  attemptEndConfigDevice(configDevice);

  configDevice = (configDevice + 1) % totalDevices;

  delay(50);

  // Turn the light on for the new device
  attemptConfigDevice(configDevice);
}

void configSetDeviceForPlayer() {
  /* Sets the current device to the current player, then moves to the next player & device. */
  Serial.print("SET: Player ");
  Serial.print(configPlayer);
  Serial.print(" to Device ID ");
  Serial.println(configDevice);

  playerDevice[configPlayer] = configDevice;

  totalPlayers++;

  // Set the colour of this device to green to indicate it has been configured succesfully.
  sendCommand(SERIAL_GREEN, macAddresses[configDevice]);

  // Increment both the player and device config values, bounding to 15 (players, with values [0, 14])

  if (configDevice == totalDevices - 1) {
    Serial.println("WARNING: Maximum devices configured. Returning to the first device...");
  }

  configDevice = (configDevice + 1) % totalDevices;
  configPlayer = (configPlayer + 1) % totalDevices;

  Serial.print("Now configuring Player ");
  Serial.println(configPlayer);

  Serial.print("Now configuring Device ");
  Serial.println(configDevice);

  attemptConfigDevice(configDevice);
}

void endConfiguration() {
  /* Sets all configured devices to green momentarily, then turns them all off. */
  for (int i = 0; i < totalDevices; i++) {
    if (playerDevice[i] != -1) {
      sendCommand(SERIAL_GREEN, getPlayerDevice(i));
    } else {
      Serial.print("Ended configuration with ");
      Serial.print(i);
      Serial.println(" devices configured.");
      break;
    }
  }

  delay(1000);
  broadcast(SERIAL_OFF);
}

/* Utility functions */

bool stringInArray(const String &value, const String array[], int size) {
  for (int i = 0; i < size; i++) {
    if (value == array[i]) return true;
  }
  return false;
}

int getSafePlayerID(String data) {
  int id = getSplitValue(data).toInt();

  if (id >= 0 && id <= 14) {
    return id;
  }

  return -1;
}

String getSplitValue(String data) {
  const char separator = ',';
  /* Splits a String by a comma, and returns everything after the first match. 
  As we are only handling inputs in the form "command,id", we at this stage do not require something more complex. */

  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return strIndex[1] > -1 ? data.substring(strIndex[0], strIndex[1]) : "";
}

uint8_t *getPlayerDevice(int id) {
  if (id < 0 || id > 14) {
    Serial.print("[ERROR] Attempted to retrieve device for unallowed player ID ");
    Serial.println(id);
    return nullptr;
  }

  return macAddresses[playerDevice[id]];
}

int modulo(int x, int N) {
    return (x % N + N) % N;
}
