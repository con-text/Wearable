/*
  Context Project BLE Login Device
*/

#define DUMP(str, i, buf, sz) { Serial.println(str); \
    for(i=0; i<(sz); ++i) { if(buf[i]<0x10) Serial.print('0'); Serial.print(char(buf[i]), HEX); } \
    Serial.println(); } //Help function for printing the Output

#define DEBUG_ENABLED

// Libraries
#include <RFduinoBLE.h>
#include <aes256.h>
#include <limits.h>
#include <FiniteStateMachine.h>
#include <elapsedMillis.h>

// Disconnect automatically after 5s
elapsedMillis interruptTimer;
#define interval 5000

// Variables
String inputString = "";
int i;

String sentRandom;
String serverRandom;
String serverCipher;
bool cipherOK = false;

// AES Encryption and Decryption
aes256_context ctxt;
uint8_t keyOne[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

uint8_t keyTwo[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

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

  uint8_t hexString[16];
  generate256BitRandom(hexString);

  String hexMessage = stringFromUInt8(hexString);

  sentRandom = hexMessage;
  Serial.println(F("Sending Random Number"));
  Serial.println(hexMessage);
  sendMessage(hexMessage);
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
    uint8_t hexString[16];

    // Convert the string to uint8
    uint8FromString(serverCipher, hexString);
    decryptMessage(0, hexString);

    // Convert the uint8 back to a string
    String decryptedString = stringFromUInt8(hexString);
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
    uint8_t hexString[16];

    // Convert the string to uint8
    uint8FromString(serverRandom, hexString);
    encryptMessage(0, hexString);

    // Convert the uint8 back to a string
    String encryptedString = stringFromUInt8(hexString);

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
  DUMP(F("Unencrypted data: "), i, data, sizeof(data) * 4);
  Serial.println(F("---Encrypting---"));
  aes256_encrypt_ecb(&ctxt, data);
  DUMP(F("Encrypted data: "), i, data, sizeof(data) * 4);
  
  // Clean up
  aes256_done(&ctxt);
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

  // Do the actual encryption
  DUMP(F("Encrypted data: "), i, data, sizeof(data) * 4);
  Serial.println(F("---Decrypting---"));
  aes256_decrypt_ecb(&ctxt, data);
  DUMP(F("Unencrypted data: "), i, data, sizeof(data) * 4);

  // Clean up
  aes256_done(&ctxt);
}

/* Utilities */

void generate256BitRandom(uint8_t* data)
{
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
}

String stringFromUInt8(uint8_t* data)
{
  String hexMessage = "";
  String newString;

  for (int i = 0; i < sizeof(data) * 4; i++) {
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
