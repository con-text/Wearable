/*
  Context Project BLE Login Device
*/

// Defines whether to use the AES crypto chip
#define AES_LIBRARY_DEBUG 1
#define interval 5000

// Proximity sensor
#define VCNL4000_ADDRESS 0x13  // 0x26 write, 0x27 read
#define COMMAND_0 0x80  // starts measurments, relays data ready info
#define PRODUCT_ID 0x81  // product ID/revision ID, should read 0x11
#define IR_CURRENT 0x83  // sets IR current in steps of 10mA 0-200mA
#define AMBIENT_PARAMETER 0x84  // Configures ambient light measures
#define AMBIENT_RESULT_MSB 0x85  // high byte of ambient light measure
#define AMBIENT_RESULT_LSB 0x86  // low byte of ambient light measure
#define PROXIMITY_RESULT_MSB 0x87  // High byte of proximity measure
#define PROXIMITY_RESULT_LSB 0x88  // low byte of proximity measure
#define PROXIMITY_FREQ 0x89  // Proximity IR test signal freq, 0-3
#define PROXIMITY_MOD 0x8A  // proximity modulator timing

// Wire library
#include <Wire.h>
// RFDuino Library
#include <RFduinoBLE.h>
// Touch sensor
#include <ADXL345.h>
// AES functions
#include <aes132.h>
#include <aes132_commands.h>
// Sizes of types
#include <limits.h>
// FSM Library
#include <FiniteStateMachine.h>
// Timing library
#include <elapsedMillis.h>
#include <Timer.h>

// Disconnect automatically after 5s
elapsedMillis interruptTimer;
Timer t;

// Variables
ADXL345 accel = ADXL345();

String serialNum = "";

String inputString = "";

String typeOfConnect;
String sentRandom;

String accountID = "";
bool setupOK = false;

bool deviceLocked = false;

String serverRandom;
String serverCipher;


bool cipherOK = false;

const int vibrationPin = 3;
const int INTERRUPT_PIN = 2;

bool isAdvertising = false;

/* State machine */
State Setup = State(initialSetupBegin, NULL, NULL);
State Advertising = State(advertising, NULL, NULL);
State PreConnect = State(resetTimer, preConnect, NULL);
State WaitForButtonInput = State(resetTimer, waitForButtonInput, NULL);
State Connected = State(didConnect);

State WaitingForID = State(resetTimer, waitingID, NULL);
State WaitingForAck = State(resetTimer, waitingAck, NULL);

State WaitingForCipher = State(resetTimer, waitingCipher, NULL);
State WaitingForRandom = State(resetTimer, waitingRandom, NULL);

State ResetVariables = State(resetVariables, NULL, NULL);

FSM stateMachine = FSM(Setup);

/* Arduino main functions */

void setup()
{
  // Setup serial
  Serial.begin(9600);
  
  // Setup vibration motor
  pinMode(vibrationPin, OUTPUT);
  pinMode(INTERRUPT_PIN, INPUT);
  
  // For I2C
  Wire.begin();
  
  // Uncomment this to wipe the current ID written to the chip
  //resetUserID();
  
  serialNum = readSerialNumber();
  accountID = readUserID();
  
  Serial.println(serialNum);
  Serial.println(accountID);
  
  //Serial.println(readZoneConfig());
 
 // Setup the proximity sensor
  byte temp = readByte(PRODUCT_ID);
  if (temp != 0x11) {
   Serial.print("Something's wrong. Not reading correct ID for proximity sensor.");
   abort();
  } 
   
  /* Now some VNCL400 initialization stuff
    Feel free to play with any of these values, but check the datasheet first!*/
  writeByte(AMBIENT_PARAMETER, 0x0F);  // Single conversion mode, 128 averages
  writeByte(IR_CURRENT, 1);  // Set IR current to 10mA
  writeByte(PROXIMITY_FREQ, 2);  // 781.25 kHz
  writeByte(PROXIMITY_MOD, 0x81);  // 129, recommended by Vishay
    
  t.every(5000, pollWearable);
  
  while (!accel.begin()) {
      Serial.println("No ADXL345 detected...");
      abort();
  } 
  
  Serial.println("Found ADXL345 :)"); 
  accel.setupTapInterrupts();
}

void loop()
{
  t.update();
  
  stateMachine.update();
}

void initialSetupBegin()
{
  Serial.println("---In initial setup---");
}

void pollWearable()
{
  int proximityValue = readProximity();
  
  Serial.println(proximityValue, DEC);
  
  // There is something close to the device
  if (proximityValue > 4000) {
    // And we're not advertising
    if (isAdvertising == false) {
      isAdvertising = true;
      stateMachine.transitionTo(Advertising);
    }
  } else {
    // Not on wrist and is advertising so stop
    if (isAdvertising == true) {
      isAdvertising = false;
      RFduinoBLE.end(); 
    }
  }  
}

/* States */

void advertising()
{
  // The AES chip should be sleeping
  aes132c_sleep();
  
  // Reset everything
  serverCipher = "";
  serverRandom = "";
  sentRandom = "";
  typeOfConnect = "";
  setupOK = false;
  
  Serial.println(F("---In advertising state---"));
      
  char dataToAdvertiseArray[accountID.length() + 1];
  accountID.toCharArray(dataToAdvertiseArray, accountID.length() + 1);
  
  // Not on wrist - not advertising
  
  // If on wrist:
  // Not setup, advertising 4E1F1FB0-95C9-4C54-88CB-6B9F3192CDD1
  // Setup but locked, advertising 79E7C777-15B4-406A-84C2-DEB389EA85E1
  // Setup and unlocked, advertising 2220

  if (!isDeviceSetup()) {
      RFduinoBLE.customUUID = "4E1F1FB0-95C9-4C54-88CB-6B9F3192CDD1";
  } else if (isDeviceLocked()) {
      RFduinoBLE.customUUID = "79E7C777-15B4-406A-84C2-DEB389EA85E1";      
  } else {
      RFduinoBLE.customUUID = "";   
  }

  // Broadcast Bluetooth
  RFduinoBLE.advertisementInterval = 200;
  RFduinoBLE.advertisementData = dataToAdvertiseArray;
  RFduinoBLE.deviceName = "Nimble";
  RFduinoBLE.begin();
}

void preConnect()
{  
  
  if ((typeOfConnect == "SETUP") && (!isDeviceSetup())) {
    Serial.println(F("---In setup state---"));
    
    Serial.println("Sending ok back..");
    sendMessage("OK");
    
    // Needs to receive ID from phone
    // Send serial number back (serialNum)
    // Upon acknowledge, write ID to chip
    
    typeOfConnect = "";
    
    stateMachine.immediateTransitionTo(WaitingForID);
    
    
  } else if (typeOfConnect == "LOGIN" && isDeviceSetup()) {
    Serial.println(F("---In login state---"));
    
    // Vibrate
    vibrate();
    
    typeOfConnect = "";

    // Wait for a button input  
    stateMachine.immediateTransitionTo(WaitForButtonInput);
  
  } else if (typeOfConnect == "HEARTBEAT" && isDeviceSetup()) {
    Serial.println(F("---In heartbeat state---"));
    stateMachine.transitionTo(Connected);
  }
  
  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive start of protocol, disconnecting."));
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
}

void waitForButtonInput()
{
  int interruptSource = accel.readInterruptSource();
   // we use a digitalRead instead of attachInterrupt so that we can use delay()
  if(digitalRead(INTERRUPT_PIN)) {
    // Weird case where we're getting all the bits set
    if ((interruptSource != 255) && (interruptSource & B00100000)) {
      Serial.println("Double tap");
      vibrateOnce();
      stateMachine.transitionTo(Connected);
    }
  }
  
  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive tap acknowledgement, cancelling."));
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
}


void didConnect()
{
  Serial.println(F("---In connected state---"));
 
  String randomString = generate128BitRandom();

  if (randomString == "ERROR")
  {
    Serial.println("Error generating random number, resetting");
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }

  sentRandom = randomString;
  Serial.println(F("Sending Random Number"));
  Serial.println(randomString);
  sendMessage(randomString);
  stateMachine.transitionTo(WaitingForCipher);
}

void resetTimer()
{
  interruptTimer = 0;
}

void waitingID()
{
  if (isDeviceSetup() == true) {
    Serial.println(F("---Received an ID from phone--"));
    
    // Send serial number as response
    sendMessage(serialNum);
    
    // Now wait for an acknowledgement before writing ID to device
    stateMachine.immediateTransitionTo(WaitingForAck);
  }
  
  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive ID, disconnecting."));
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
}

void waitingAck()
{
  if ((setupOK == true) && (accountID != "Nimble")) {
    Serial.println(F("---Received an Ack from phone--"));

    // Write the UserID provided by the phone
    writeUserID(accountID);
    
    delay(100);
    
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
  
  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive acknowledgment, disconnecting."));
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
}

void waitingCipher()
{
  if (serverCipher != "") {
    Serial.println(F("---Decoding received cipher--"));
    
    // Hardware has a MAC
    uint8_t hexString[32];
    uint8_t data[16];
    uint8_t MAC[16];

    // Convert the string to uint8
    uint8FromString(serverCipher, hexString);
    
    memcpy(&MAC, &hexString, 16);
    memcpy(&data, &hexString[16], 16);  
    String decryptedString = decryptMessage(0, data, MAC);
    
    if (decryptedString == "ERROR")
    {
      Serial.println("Error generating cipher, resetting");
      RFduinoBLE.end();
      stateMachine.transitionTo(Advertising);
    }

    // Convert the uint8 back to a string
    Serial.println(decryptedString);

    if (decryptedString == sentRandom) {
      Serial.println(F("Got the correct random value"));
      sendMessage("OK");
      stateMachine.transitionTo(WaitingForRandom);
    } else {
      Serial.println(F("Incorrect random, fuck off server"));
      RFduinoBLE.end();
      stateMachine.transitionTo(Advertising);
    }
  }

  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive ciphertext, disconnecting."));
    RFduinoBLE.end();
    stateMachine.transitionTo(Advertising);
  }
}

void waitingRandom()
{
  if (serverRandom != "") {
    Serial.println(F("---Encrypting received random--"));
    // Hardware has a MAC     
    uint8_t hexString[16];

    // Convert the string to uint8
    uint8FromString(serverRandom, hexString);
    
    String encryptedString = encryptMessageString(0, hexString);

    if (encryptedString == "ERROR")
    {
      Serial.println("Error generating cipher, resetting");
      RFduinoBLE.end();
      stateMachine.transitionTo(Advertising);
    }
    // Convert the uint8 back to a string
    sendMessage(encryptedString);
    Serial.println("Sending encrypted string");
    Serial.println(encryptedString);

    stateMachine.transitionTo(ResetVariables);
  }

  if (interruptTimer > interval) {
    Serial.println(F("Didn't receive random challenge, disconnecting."));
    RFduinoBLE.end();
    stateMachine.transitionTo(ResetVariables);
  }
}

void resetVariables()
{
  // Sleep the AES chip
  aes132c_standby();
  
  Serial.println(F("---In reset state---"));

  serverCipher = "";
  serverRandom = "";
  sentRandom = "";
  typeOfConnect = "";
  setupOK = false;
}


/* RFDuino Callbacks */

void RFduinoBLE_onConnect()
{
  Serial.println(F("Got connection"));

  stateMachine.transitionTo(PreConnect);
}

void RFduinoBLE_onDisconnect()
{
  Serial.println(F("Disconnected"));
}

/* Sending and receiving */

void receivedMessage(String message)
{
  Serial.println(F("---Received message--"));
  Serial.println(message);

  if (stateMachine.isInState(WaitingForCipher)) {
    serverCipher = message;
  } else if (stateMachine.isInState(WaitingForRandom)) {
    serverRandom = message;
  } else if (stateMachine.isInState(PreConnect)) {
    typeOfConnect = message;
  } else if (stateMachine.isInState(WaitingForID)) {
    accountID = message; 
    Serial.println("Still in state");
  } else if (stateMachine.isInState(WaitingForAck)) {
    Serial.println("Setting oK");
    if (message == "OK") setupOK = true;    
  } else {
    Serial.println(F("Unknown state"));
  }
}

/* RFduino sending and receiving */
void sendMessage(String messageToSend)
{
  // The MTU for RFDuino is 20 bytes
  int stringLength = messageToSend.length();

  String currentSubMessage = "1";
  int msgSize = 19;
  if (stringLength < 19) msgSize = stringLength;

  currentSubMessage += messageToSend.substring(0, msgSize + 1);

  char currentSubMessageArray[currentSubMessage.length() + 1];
  currentSubMessage.toCharArray(currentSubMessageArray, currentSubMessage.length() + 1);

  while (!RFduinoBLE.send(currentSubMessageArray, msgSize + 1));

  int messagesToSend = ceil((float)stringLength / (float)19);

  for (int i = 1; i < messagesToSend; i++) {
    currentSubMessage = "2";

    if ((i * 19 + 19) > stringLength) {
      msgSize = stringLength - (i * 19);
    }

    currentSubMessage += messageToSend.substring(i * 19, (i * 19) + msgSize + 1);

    char currentSubMessageArray[currentSubMessage.length() + 1];
    currentSubMessage.toCharArray(currentSubMessageArray, currentSubMessage.length() + 1);
    while (!RFduinoBLE.send(currentSubMessageArray, msgSize + 1));
  }

  while (!RFduinoBLE.send('3'));
}

void RFduinoBLE_onReceive(char *data, int len)
{
  // Start of message
  if (data[0] == '1') {
    inputString = "";
    for (int i = 1; i < len; i++) {
      inputString += data[i];
    }
  }

  // Data
  if (data[0] == '2') {
    for (int i = 1; i < len; i++) {
      inputString += data[i];
    }
  }

  // EOM
  if (data[0] == '3') {
    if (inputString != "") {
      receivedMessage(inputString);
    }
  }
}

/* AES Hardware functions */
String generate128BitRandom()
{
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 16] = {0};
  randomNumber(rxBuffer);

  if (rxBuffer[1] != 0x00) {
    return "ERROR";
  }

  // Remove the packet size and checksums
  cleanupData(rxBuffer, sizeof(rxBuffer));

  // Turn the data array into a string
  String randomString = stringFromUInt8(rxBuffer, sizeof(rxBuffer));

  return randomString;
}

String decryptMessage(int key, uint8_t dataToDecrypt[16], uint8_t inMac[16])
{
  // Send the nonce
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer);
  
  if (rxBuffer[1] != 0x00) {
    return "ERROR";
  }
  
  // Send the decrypt
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN+16] = {0};
  decrypt(inMac, dataToDecrypt, rxBuffer2);
      
  if (rxBuffer2[1] != 0x00) {
    return "ERROR";
  }
  
  // Remove the packet size and checksums
  cleanupData(rxBuffer2, sizeof(rxBuffer2));
    
  // Turn the data array into a string
  String decryptedMessage = stringFromUInt8(rxBuffer2, sizeof(rxBuffer2));

  return decryptedMessage;
}

String encryptMessageString(int key, uint8_t data[16])
{
  // Send the nonce
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer);
  
  if (rxBuffer[1] != 0x00) {
    return "ERROR";
  }
  
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN + 32] = {0};
  encrypt(data, rxBuffer2);
    
  if (rxBuffer2[1] != 0x00) {
    return "ERROR";
  }
  
  // Remove the packet size and checksums
  cleanupData(rxBuffer2, sizeof(rxBuffer2));
    
  // Turn the data array into a string
  String encryptedMessage = stringFromUInt8(rxBuffer2, sizeof(rxBuffer2));

  return encryptedMessage;
}

/* Vibration Motor */
void vibrate()
{
  digitalWrite(vibrationPin, HIGH);
  delay(100);
  digitalWrite(vibrationPin, LOW);
  delay(100);
  digitalWrite(vibrationPin, HIGH);
  delay(100);
  digitalWrite(vibrationPin, LOW);
}

void vibrateOnce()
{
  digitalWrite(vibrationPin, HIGH);
  delay(100);
  digitalWrite(vibrationPin, LOW);
}

/* Utilities */

String readSerialNumber()
{
  uint8_t serialNumber[AES132_RESPONSE_SIZE_MIN + 8] = {0};
  uint16_t address = 0xF000;
   
  aes132m_block_read(address, 8, serialNumber);
  
  return stringFromUInt8(serialNumber, AES132_RESPONSE_SIZE_MIN + 8);
}

String readZoneConfig()
{
  uint8_t serialNumber[AES132_RESPONSE_SIZE_MIN + 4] = {0};
  uint16_t address = 0xF0C0;
   
  aes132m_block_read(address, 4, serialNumber);
  
  return stringFromUInt8(serialNumber, AES132_RESPONSE_SIZE_MIN + 4);
}


String readUserMemory(uint16_t address)
{
  uint8_t serialNumber[AES132_RESPONSE_SIZE_MIN + 25] = {0};
   
  aes132m_block_read(address, 25, serialNumber);
  
  return stringFromUInt8(serialNumber, AES132_RESPONSE_SIZE_MIN + 25);
}

void cleanupData(uint8_t *data, int dataLength)
{
  data[0] = NULL;
  data[dataLength - 1] = NULL;
  data[dataLength - 2] = NULL;
}

String stringFromUInt8(uint8_t* data, int dataLength)
{
  String hexMessage = "";
  String newString;

  for (int i = 0; i < dataLength; i++) {
    
    // An edge case because the data that has been cleaned up has been nulled, but the data could also be null
    if (data[i] == NULL) {
      if ( (i == 0) || (i == 1) || (i == dataLength - 1) || (i == dataLength - 2)) {
        continue;
      }
      hexMessage += "00";
      continue;
    }
    
    newString = "";
    // Special case for 0
    if (data[i] < 0x10)
      newString += "0";
    newString += String(char(data[i]), HEX);
    hexMessage += newString;
  }

  hexMessage.toUpperCase();

  return hexMessage;
}

void uint8FromString(String message, uint8_t* data)
{
  for (int i = 0; i < message.length(); i = i + 2) {
    String internalString = message.substring(i, i + 2);
    char buf[2];
    internalString.toCharArray(buf, 2 + 1);
    data[i / 2] = strtol(buf, NULL, 16);
  }
}

void PrintHex8(const __FlashStringHelper* message, uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
  Serial.println(message);
  Serial.print("0x");
  for (int i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}

// writeByte(address, data) writes a single byte of data to address
void writeByte(byte address, byte data)
{
  Wire.beginTransmission(VCNL4000_ADDRESS);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
}

// readByte(address) reads a single byte of data from address
byte readByte(byte address)
{
  byte data;
  
  Wire.beginTransmission(VCNL4000_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(VCNL4000_ADDRESS, 1);
  while(!Wire.available())
    ;
  data = Wire.read();

  return data;
}

// readProximity() returns a 16-bit value from the VCNL4000's proximity data registers
unsigned int readProximity()
{
  unsigned int data;
  byte temp;
  
  temp = readByte(COMMAND_0);
  writeByte(COMMAND_0, temp | 0x08);  // command the sensor to perform a proximity measure
  
  while(!(readByte(COMMAND_0)&0x20)) 
    ;  // Wait for the proximity data ready bit to be set
  data = readByte(PROXIMITY_RESULT_MSB) << 8;
  data |= readByte(PROXIMITY_RESULT_LSB);
  
  return data;
}

void writeUserID(String accID) {
  
  char userID[accID.length() + 1];
  accID.toCharArray(userID, accID.length() + 1);
  
  //char userID[] = "10155232305430398";
  //char userID[] = "1234567";
  
  // Stored in ASCII representation e.g. 0 = 30, 1 = 31, 2 = 32 etc.
  uint16_t address = 0x0000;
  
  // Make array large enough to hold
  // Length (2 bytes)
  // Empty byte 0x00
  // UserID in ASCII, one character at a time
  uint8_t uid[sizeof(userID) + 3];
  
  String uidLength = String(strlen(userID));
  uid[0] = uidLength[0];
  uid[1] = uidLength[1];
  uid[2] = 0x00;
  
  for (int i = 0; i < strlen(userID); i++) {
    uid[i+3] = userID[i];
  }
 
  aes132c_write_memory(sizeof(userID) + 3 - 1, address, uid);

}

void resetUserID() {
  uint16_t address = 0x0000;
  uint8_t uid[10];
  
  for (int i = 0; i < 10; i++) {
    uid[i] = 0xFF;
  }
  
  aes132c_write_memory(10, address, uid);
}

String readUserID() {
  
  // Read length of UserID from start of User Zone 0
  uint16_t address = 0x0000;
  uint8_t userIDLength[AES132_RESPONSE_SIZE_MIN + 2] = {0};
  aes132m_block_read(address, 2, userIDLength);
  
  String userIDString = stringFromUInt8(userIDLength, AES132_RESPONSE_SIZE_MIN + 2);
  
  // No UserID has been written yet, must be a new device
  if (userIDString[2] == 'F') {
    return String("Nimble");
  } 
  
  // If a UserID has been written, read the correct number of characters
  int userIDchars = 0;
  userIDchars = asciiToNumerical(userIDString).toInt();
  
  address += 3;
  uint8_t userIDStr[AES132_RESPONSE_SIZE_MIN + userIDchars];
  memset( userIDStr, 0, (AES132_RESPONSE_SIZE_MIN + userIDchars)*sizeof(uint8_t) );
   
  aes132m_block_read(address, userIDchars, userIDStr);
  
  return asciiToNumerical(stringFromUInt8(userIDStr, AES132_RESPONSE_SIZE_MIN + userIDchars));
}

String asciiToNumerical(String asciiString) {
  
  String trimmedPacket = asciiString.substring(2,asciiString.length()-4);
  String fullString; 
   
  for (int i = 0; i < trimmedPacket.length(); i = i + 2) {
    fullString += String(trimmedPacket.substring(i,i+2).toInt()-30);
  }
  
  return fullString;
}

bool isDeviceSetup() {
  if ((accountID != "") && (accountID != "Nimble")) {
    return true;
  }
  else {
    return false;
  }
}

bool isDeviceLocked() {
  return deviceLocked;
}

