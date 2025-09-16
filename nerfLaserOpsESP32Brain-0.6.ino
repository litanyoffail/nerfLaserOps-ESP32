#define BUTTON_RIGHT 35

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
//{0x24, 0xDC, 0xC3, 0x98, 0x79, 0x64}; // Original Dev Board MAC addresses
//{0x24, 0xDC, 0xC3, 0x98, 0xA0, 0xDC}; // saved in case they are added later
const uint8_t broadcastAddress[][6] = {{0xA0, 0xA3, 0xB3, 0x29, 0xFD, 0xBC}, 
                                      {0xA0, 0xA3, 0xB3, 0x2A, 0x09, 0xF4}, 
                                      {0xA0, 0xA3, 0xB3, 0x2C, 0x6E, 0x90}};

// Variable to store if sending data was successful
String success;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  uint8_t targetID;
  uint8_t gameType;
  uint8_t targetDelay;
  bool isTarget;
  bool isTargetHit;
  uint8_t hitBy;
  uint32_t millisScore;
} struct_message;

struct_message myData;

struct_message target1Data;
struct_message target2Data;
struct_message target3Data;
// struct_message target4;

struct_message targetStruct[3] = {target1Data, target2Data, target3Data};

esp_now_peer_info_t peerInfo;

uint8_t gameType = 0;

// Arrays for colors: purple/red/blue
uint8_t hitScore[] = {0, 0, 0};
uint32_t millisScore[] = {0, 0, 0};

uint8_t missScore = 0;

// Flags for active targets: Target 1 / Target 2 / Target 3
bool isTarget[] = {0, 0, 0};

uint8_t targetCycles = 0;

uint32_t millisPrevious;
uint32_t millisCurrent;

// Delays for activating targets in a random pattern.
uint8_t targetPattern[12][3] = {{0, 0, 0},
                                {0, 0, 0},
                                {0, 1, 2},
                                {0, 1, 2},
                                {2, 1, 0},
                                {2, 1, 0},
                                {2, 0, 1},
                                {1, 0, 2},
                                {1, 0, 1},
                                {2, 0, 2},
                                {0, 1, 1},
                                {1, 1, 0}};

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
// Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Target ID %u: %u bytes\n", myData.targetID, len);
  updateTargetData(myData.targetID, myData.hitBy);
}

void updateTargetData(uint8_t id, uint8_t hitBy) {
// Update target data structures with the new incoming data.
  targetStruct[id].targetID = myData.targetID;
  targetStruct[id].isTarget = myData.isTarget;
  targetStruct[id].isTargetHit = myData.isTargetHit;
  targetStruct[id].hitBy = hitBy;
  targetStruct[id].millisScore = myData.millisScore;
  Serial.printf("Hit By: %d \n", targetStruct[id].hitBy);
  Serial.println();
  Serial.printf("Milliseconds: %d \n", targetStruct[id].millisScore);
  Serial.println();
// Update score variables with received data.
  switch (targetStruct[id].isTargetHit) {
    case 1:
      hitScore[hitBy]++;
      millisScore[hitBy] = millisScore[hitBy] + targetStruct[id].millisScore;
      break;
    case 0:
      Serial.println("Miss!");
      for (uint8_t i = 0; i < 3; i++){
        millisScore[i] = millisScore[i] + 1200;
      }
      missScore++;
      break;
  }
// Update target status
  isTarget[id] = targetStruct[id].isTarget;
}

// Function to send data to target units to trigger target status
void targetActivate(uint8_t id, uint8_t delay) {
  isTarget[id] = 1;
  myData.targetID = id;
  myData.gameType = gameType;
  myData.targetDelay = delay+1;
  myData.isTarget = 1;
    esp_err_t result = esp_now_send(broadcastAddress[id],
    (uint8_t *) &myData,
    sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

// Turn off all targets at once but preserve gameType status.
void targetDeactivateAll () {
    myData.gameType = gameType;
    myData.isTarget = 0;
    esp_err_t result = esp_now_send(0,
    (uint8_t *) &myData,
    sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

// Screen animation to start game.
void gameStart(uint8_t mode) {
  gameType = mode;
  tft.setTextSize(12);
  tft.setCursor(0, 0);
  tft.print("Start...");
  delay(2000);
  for (int t = 3; t >= 0; t--) {
    tft.fillScreen(0x0000);
    tft.print(t);
    delay(1000);
  }
  tft.fillScreen(0x0000);
  tft.print("GO!");
}

// Set gameType to 0 to turn off all targets; will not trigger a "miss" as
// above, then display statistics for each color blaster for 20s
void gameOver() {
  gameType = 0;
  myData.gameType = 0;
    esp_err_t result = esp_now_send(0,
    (uint8_t *) &myData,
    sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Millis");
  tft.print("Purple: ");
  tft.println(millisScore[0]);
  tft.print("Misses: ");
  tft.println(missScore);
  delay(20000);
}

void setup() {
  delay(5000);
  Serial.begin(115200);
//  while (!Serial) {};
// Initialize Wi-Fi and ESP-NOW
  pinMode(BUTTON_RIGHT, INPUT);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
// Once ESPNow is successfully Init, we will register for Send CB to
// get the status of transmitted packet
  esp_now_register_send_cb(OnDataSent);
// Register peers
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
// first peer
  memcpy(peerInfo.peer_addr, broadcastAddress[0], 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
    }
// second peer  
  memcpy(peerInfo.peer_addr, broadcastAddress[1], 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
// third peer
  memcpy(peerInfo.peer_addr, broadcastAddress[2], 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

// Add more peers in the same format to add more targets.

//  memcpy(peerInfo.peer_addr, broadcastAddress[3], 6);
//  if (esp_now_add_peer(&peerInfo) != ESP_OK){
//    Serial.println("Failed to add peer");
//    return;
//  }

// Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  tft.begin();
  tft.fillScreen(0x0000);
  tft.setRotation(1);
  Serial.println("Ready to Start");
}

void loop() {
// gameTypes
// off = 0
// Sequential Random Targets = 1
// Patterned Random Targets = 2
  switch (gameType) {
    case 0:
      tft.fillScreen(0x0000);
      tft.setTextSize(3);
      tft.setCursor(0, 0);
      tft.println("Press Button to Start Game");
// Stop the loop until a game type is chosen
    while (gameType == 0) {
      if (digitalRead(BUTTON_RIGHT) == LOW) {
        gameStart(1);
      }
    }
      break;
    case 1:
// Grab current time, then check that all targets are Off and activate one
// at random; wait, and if set time is reached turn off all targets
// manually; this will trigger a "miss" as above.
// Trigger Game Over after set number of target cycles.
      millisCurrent = millis();
      if (targetCycles > 50) {
        gameOver();
      }
      else if (isTarget[0] == 0 &&
                isTarget[1] == 0 &&
                isTarget[2] == 0 &&
                targetCycles <= 50) {
        targetActivate(random(3), 0);
        millisPrevious = millis();
        targetCycles++;
      }
      else if (millisCurrent - millisPrevious >= 1550) {
        targetDeactivateAll();
        millisPrevious = millis();
      }
      break;
    case 2:
// grab current time then if all targets are off: randomly select a pattern
// and send to all targets with delay multiplier. if set time is reached,
// turn off all targets manually; this will trigger a "miss" as above.
// Trigger Game Over after set number of target cycles.
      millisCurrent = millis();
      if (targetCycles > 18) {
        gameOver();
      }
      else if (isTarget[0] == 0 &&
                isTarget[1] == 0 &&
                isTarget[2] == 0 &&
                targetCycles <= 18) {
        for (uint8_t i = 0; i < 3; i++) {
          targetActivate(i, targetPattern[random(13)][i]);
        }
        millisPrevious = millis();
        targetCycles++;
      }
      else if (millisCurrent - millisPrevious >= 2250) {
        targetDeactivateAll();
        millisPrevious = millis();
      }
      break;
  }
}
