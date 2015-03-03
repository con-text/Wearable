/*
  Context Project BLE Login Device
*/

// Defines whether to use the AES crypto chip
#define interval 5000

// Wire library
#include <Wire.h>
// RFDuino Library
#include <RFduinoBLE.h>

// AES functions
#include <aes132.h>
#include <aes132_commands.h>
// Sizes of types
#include <limits.h>
// FSM Library
#include <FiniteStateMachine.h>
// Timing library
#include <elapsedMillis.h>

// Disconnect automatically after 5s
elapsedMillis interruptTimer;

// Variables
String inputString = "";

String typeOfConnect;
String sentRandom;
String serverRandom;
String serverCipher;
bool cipherOK = false;

const int vibrationPin = 3;

/* State machine */
State Setup = State(firstSetup, NULL, NULL);
State Advertising = State(advertising, NULL, NULL);
State PreConnect = State(preConnect);
State Connected = State(didConnect);
State WaitingForCipher = State(resetTimer, waitingCipher, NULL);
State WaitingForRandom = State(resetTimer, waitingRandom, NULL);
State ResetVariables = State(resetVariables, NULL, NULL);

FSM stateMachine = FSM(Advertising);

/* Arduino main functions */

void setup()
{
  // Setup serial
  Serial.begin(9600);
  
  // Setup vibration motor
  pinMode(vibrationPin, OUTPUT);
  
  // For I2C
  Wire.begin();
  
  delay(100);
  
  Serial.println(readSerialNumber());
  
  // Test the AES
  // DUMP("KEY One: ", i, keyOne, sizeof(keyOne));
  // DUMP("KEY Two: ", i, keyTwo, sizeof(keyTwo));
}

void loop()
{
  //stateMachine.update();
}

/* States */

void firstSetup()
{
  
  
}

void advertising()
{
  // The AES chip should be sleeping
  aes132c_sleep();
  // Reset everything
  serverCipher = "";
  serverRandom = "";
  sentRandom = "";
  typeOfConnect = "";
  
  Serial.println(F("---In advertising state---"));

  // Broadcast Bluetooth
  RFduinoBLE.advertisementData = "EA8F2A44";
  RFduinoBLE.deviceName = "Context";
  RFduinoBLE.begin();
}

void preConnect()
{  
  if (typeOfConnect == "LOGIN") 
  {
    Serial.println(F("---In login state---"));
    // Vibrate
    vibrate();
  
    //TODO: We need to listen for a button input at this point    
    stateMachine.transitionTo(Connected);

  } else if (typeOfConnect == "HEARTBEAT") {
    Serial.println(F("---In heartbeat state---"));
    stateMachine.transitionTo(Connected);
  }
}

void didConnect()
{
  Serial.println(F("---In connected state---"));
 
  String randomString = generate128BitRandom();

  if (randomString == "ERROR")
  {
    Serial.println("Error generating random number, resetting");
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
 // stateMachine.transitionTo(ResetVariables);
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
  delay(250);
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
