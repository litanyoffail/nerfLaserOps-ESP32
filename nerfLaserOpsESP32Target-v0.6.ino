#define IR_RECEIVER 16 // IR receiver pin
#define LEDFLASHBUILTIN 4 // Target Status LED PIN

//First create a decoder that just returns a 32 bit hash.
#include <IRLibDecodeBase.h> 
#include <IRLib_HashRaw.h>  //Must be last protocol
#include <IRLibCombo.h>     // After all protocols, include this
// All of the above automatically creates a universal decoder
// class called "IRdecode" containing only the protocols you want.
// Now declare an instance of that decoder.
IRdecode myDecoder;

// Include a receiver either this or IRLibRecv or IRLibRecvPCI; Loop is
// used here as the brain unit is doing all timing control
#include <IRLibRecvLoop.h> 
IRrecvLoop myReceiver(IR_RECEIVER);    //create a receiver on the IR IN pin

#include <esp_now.h>
#include <WiFi.h>

// IR hashes for Nerf LaserOps Out-Of-Box Play:
// AP "AlphaPoint" & Classic "Ion"
#define AP_PURPLE 0x67228B44
#define AP_RED 0x78653B0E 
#define AP_BLUE 0x2FFEA610
// DB "DeltaBurst"
#define DB_PURPLE 0xD303E9B8
#define DB_RED 0xE4469982
#define DB_BLUE 0x9BE00484

const uint32_t alphaPoint[] = {AP_PURPLE, AP_RED, AP_BLUE};
const uint32_t deltaBurst[] = {DB_PURPLE, DB_RED, AP_BLUE};

uint8_t targetID;
uint8_t gameType = 0;
bool isTarget = 0;
uint32_t millisPrevious;
uint32_t millisCurrent;
uint32_t millisScore;

// MAC address for the brain module {0x3C, 0xE9, 0x0E, 0x56, 0xBC, 0x74}
const uint8_t broadcastAddress[] = {0x3C, 0xE9, 0x0E, 0x56, 0xBC, 0x74};

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

struct_message gameData;

esp_now_peer_info_t peerInfo;

// Check incoming IR hash code against valid blasters for the game type.
// Will add special types where only certain blasters work in the future.
bool isValidHit() {
  myDecoder.decode();
  switch (gameType) {
    case 0:
      return false;
      break;
    case 1: case 2:
      return myDecoder.protocolNum == UNKNOWN && 
        (myDecoder.value == AP_PURPLE ||
        myDecoder.value == AP_RED ||
        myDecoder.value == AP_BLUE ||
        myDecoder.value == DB_PURPLE ||
        myDecoder.value == DB_RED ||
        myDecoder.value == DB_BLUE) &&
        isTarget == 1;
      break;
  }
}

// actions when a valid hit is received; collects and sends data
// to the brain to tell it what target was hit, what color blaster
// hit the target, how many millis the target was active, and that
// it is no longer a target.
void validHitResponse() {
  millisCurrent = millis();
  analogWrite(LEDFLASHBUILTIN, 0);
  Serial.print("VALID HIT 0x");
  Serial.println(myDecoder.value, HEX);
  millisScore = millisCurrent - millisPrevious;
  Serial.print("Milliseconds ");
  Serial.println(millisScore);
  myData.targetID = targetID;
  myData.isTarget = 0;
  myData.isTargetHit = 1;
  switch (myDecoder.value) {
    case AP_PURPLE: case DB_PURPLE:
      myData.hitBy = 0;
      break;
    case AP_RED: case DB_RED:
      myData.hitBy = 1;
      break;
    case AP_BLUE: case DB_BLUE:
      myData.hitBy = 2;
      break;
  }
  myData.millisScore = millisScore;
    esp_err_t result = esp_now_send(
    broadcastAddress, 
    (uint8_t *) &myData,
    sizeof(struct_message));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  isTarget = 0;
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else {
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&gameData, incomingData, sizeof(gameData));
  Serial.print("Bytes received: ");
  Serial.println(len);
// Set targetID based on list of broadcast addresses in the "brain". This
// could be hard-coded, though this method allows flashing the same code to
// all targets without modifying the ID before doing so.
  if (targetID != gameData.targetID) {
    targetID = gameData.targetID;
  }
// check if the game type has changed, and if the game is over while the
// target is active, deactivate the target without reporting a miss.
  if (gameType != gameData.gameType) {
    gameType = gameData.gameType;
    if (gameData.gameType == 0) {
      analogWrite(LEDFLASHBUILTIN, 0);
      isTarget = 0;
    }
  }
// activate target with scaling delay as received
  if (gameData.gameType != 0 && gameData.isTarget == 1 && isTarget == 0) {
    delay(gameData.targetDelay * 350);
    analogWrite(LEDFLASHBUILTIN, 5);
    isTarget = 1;
    millisPrevious = millis();
  }
// if target is deactivated before a hit is registered, send packet to the
// brain to log a "miss"
  if (gameData.gameType != 0 && gameData.isTarget == 0 && isTarget == 1) {
    analogWrite(LEDFLASHBUILTIN, 0);
    isTarget = 0;
    myData.targetID = targetID;
    myData.isTargetHit = 0;
    myData.millisScore = 1200;
      esp_err_t result = esp_now_send(
      broadcastAddress, 
      (uint8_t *) &myData,
      sizeof(struct_message));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}

void setup() {
  delay(5000);
  Serial.begin(115200);
  pinMode(LEDFLASHBUILTIN, OUTPUT);
// Set device as a Wi-Fi Station & initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
// Once ESPNow is successfully initialized, register for 
// Send callback to get the status of transmitted packet
  esp_now_register_send_cb(OnDataSent);
// Register peer, the brain module in this case
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
// report if peer connection fails
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
// Register for receive callback when data is received
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
// Start IR receiver
  myReceiver.enableIRIn();
  Serial.println("Ready to receive IR signals");
}

void loop() {
// wait for incoming IR signals, check if valid, then reinit IR receiver
  if (myReceiver.getResults()) {
    if (isValidHit()) {
      validHitResponse();
    }
    else {
      Serial.print("INVALID HIT 0x");
      Serial.println(myDecoder.value, HEX);
    }
    myReceiver.enableIRIn();
  }
}
