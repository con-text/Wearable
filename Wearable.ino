/*
  Context Project BLE Login Device
*/


// Defines whether to use the AES crypto chip
#define AES_HARDWARE 1
#define AES_LIBRARY_DEBUG 1
#define interval 5000

// Wire library
#include <Wire.h>
// RFDuino Library
#include <RFduinoBLE.h>

// AES functions
#if AES_HARDWARE
  #include <aes132.h>
  #include <aes132_commands.h>
#else
  #include <aes256.h>
#endif
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

String sentRandom;
String serverRandom;
String serverCipher;
bool cipherOK = false;

// AES Encryption and Decryption
#if AES_HARDWARE == 0
  aes256_context ctxt;
  
  uint8_t keyOne[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };

  uint8_t keyTwo[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };

#endif

/* State machine */
State Advertising = State(advertising, NULL, NULL);
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

  // Test the AES
  // DUMP("KEY One: ", i, keyOne, sizeof(keyOne));
  // DUMP("KEY Two: ", i, keyTwo, sizeof(keyTwo));
}

void loop()
{
  stateMachine.update();
}

/* States */

void advertising()
{
  // Reset everything
  serverCipher = "";
  serverRandom = "";
  sentRandom = "";

  Serial.println(F("---In advertising state---"));

  // Broadcast Bluetooth
  RFduinoBLE.advertisementData = "EA8F2A44";
  RFduinoBLE.deviceName = "Context";
  RFduinoBLE.begin();
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
    #if AES_HARDWARE
      uint8_t hexString[32];
      uint8_t data[16];
      uint8_t MAC[16];
    #else
      uint8_t hexString[16];
    #endif

    // Convert the string to uint8
    uint8FromString(serverCipher, hexString);
    
    #if AES_HARDWARE
      memcpy(&MAC, &hexString, 16);
      memcpy(&data, &hexString[16], 16);  
      String decryptedString = decryptMessage(0, data, MAC);
    #else
      decryptMessage(0, hexString);
      String decryptedString = stringFromUInt8(hexString, 16);
    #endif
    
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
    
    #if AES_HARDWARE
      String encryptedString = encryptMessageString(0, hexString);
    #else
      encryptMessage(0, hexString);
      String encryptedString = stringFromUInt8(hexString, 16);
    #endif

    if (encryptedString == "ERROR")
    {
      Serial.println("Error generating cipher, resetting");
      stateMachine.transitionTo(Advertising);
    }
    // Convert the uint8 back to a string
    sendMessage(encryptedString);
    Serial.println("Sending encrypted string");

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
  Serial.println(F("---In reset state---"));

  serverCipher = "";
  serverRandom = "";
  sentRandom = "";
}


/* RFDuino Callbacks */

void RFduinoBLE_onConnect()
{
  stateMachine.transitionTo(Connected);
}

void RFduinoBLE_onDisconnect()
{
  Serial.println(F("Disconnected"));
  //stateMachine.immediateTransitionTo(Advertising);
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

/* AES Encryption and Decryption */

#if AES_HARDWARE

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
  decrypt(inMac, dataToDecrypt, rxBuffer);
  
  if (rxBuffer[1] != 0x00) {
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

#else

/* AES Software functions */

String generate128BitRandom()
{
  uint8_t data[16];

  unsigned long randomValue = micros();
  randomSeed(randomValue);

  long r = random(LONG_MAX);
  long r1 = random(LONG_MAX);
  long r2 = random(LONG_MAX);
  long r3 = random(LONG_MAX);

  memcpy(data, &r, sizeof(long));
  memcpy(data + 4, &r1, sizeof(long));
  memcpy(data + 8, &r2, sizeof(long));
  memcpy(data + 12, &r3, sizeof(long));

  // Turn the data array into a string
  String randomString = stringFromUInt8(data, sizeof(data));
  
  return randomString;
}

void decryptMessage(int key, uint8_t data[16])
{
  // Setup the keys
  if (key == 0) {
    aes256_init(&ctxt, keyOne);
  } else if (key == 1) {
    aes256_init(&ctxt, keyTwo);
  } else {
    Serial.println(F("Unknown key specified"));
  }

  // Do the actual decryption
  PrintHex8(F("Encrypted data: "), data, sizeof(data) * 4);
  Serial.println(F("---Decrypting---"));
  aes256_decrypt_ecb(&ctxt, data);
  PrintHex8(F("Unencrypted data: "), data, sizeof(data) * 4);

  // Clean up
  aes256_done(&ctxt);
}

void encryptMessage(int key, uint8_t data[16])
{
  // Setup the keys
  if (key == 0) {
    aes256_init(&ctxt, keyOne);
  } else if (key == 1) {
    aes256_init(&ctxt, keyTwo);
  } else {
    Serial.println(F("Unknown key specified"));
  }

  // Do the actual encryption
  PrintHex8(F("Unencrypted data: "), data, sizeof(data) * 4);
  Serial.println(F("---Encrypting---"));
  aes256_encrypt_ecb(&ctxt, data);
  PrintHex8(F("Encrypted data: "), data, sizeof(data) * 4);

  // Clean up
  aes256_done(&ctxt);
}

#endif

/* Utilities */

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
    if (data[i] == NULL) {
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
