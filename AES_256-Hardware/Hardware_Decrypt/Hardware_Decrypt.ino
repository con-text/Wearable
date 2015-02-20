#include <Wire.h>
#include <aes132.h>
//#include <aes132_i2c.h>
#include <aes132_commands.h>
#define AES_LIBRARY_DEBUG 0

byte val = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  Wire.begin();
  
  String randomString = generate128BitRandom();
  
  Serial.println(randomString);
  
  /* UNCOMMENT FOR DECRYPTION */
/*  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer2);
  
  for (int i = 0; i < sizeof(rxBuffer2); i++) {
    Serial.println(rxBuffer2[i], HEX);
  }
  
  uint8_t dataToDecrypt[] = {
    0xD5, 0x02, 0x16, 0x39, 0x56, 0x0E, 0x28, 0x29, 
    0xF2, 0x97, 0x0F, 0x06, 0x18, 0x78, 0x52, 0xDF };
    
    
  uint8_t inMac[] = {  
    0x27, 0x39, 0x6A, 0xBC, 0xC2, 0xFD, 0x88, 0x2C, 
    0x15, 0xC7, 0xE5, 0x0D, 0x0B, 0x62, 0x6E, 0x24 };
    
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN+16] = {0};
  decrypt(inMac, dataToDecrypt, rxBuffer);
    
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  }*/
   
      
 /* UNCOMMENT FOR WRITING KEYS 
   uint8_t keyOne[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    
  writeKey(keyOne, 0);
 // uint16_t address = 0xF200;
 // uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 16] = {0};
  //aes132m_block_read(address, 16, rxBuffer);
  
  /* Uncomment for generating nonce 
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer);
  delay(100);
  
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  } */
  
  /* Uncomment for encryption
  
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer2);
  
  for (int i = 0; i < sizeof(rxBuffer2); i++) {
    Serial.println(rxBuffer2[i], HEX);
  }
  
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 32] = {0};
  encrypt(rxBuffer);
    
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  } */
 
   //getInfo();
   //calculateTemperature();
   
   //testWriteRead();
}

void loop() {
  // put your main code here, to run repeatedly:

}

String generate128BitRandom()
{
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN+16] = {0};
  randomNumber(rxBuffer);
  
  if (rxBuffer[1] != 0x00) {
    return "ERROR";
  } 
  
  // Remove the packet size and checksums
  cleanupData(rxBuffer, sizeof(rxBuffer));
    
  String randomString = stringFromUInt8(rxBuffer, sizeof(rxBuffer));
  
  return randomString;
}

void cleanupData(uint8_t *data, int dataLength)
{
  data[0] = NULL;
  data[dataLength-1] = NULL;
  data[dataLength-2] = NULL; 
}

/* Utility functions */

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

void PrintHex8(String message, uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
  Serial.println(message);
  Serial.print("0x"); 
  for (int i=0; i<length; i++) { 
    if (data[i]<0x10) {
      Serial.print("0");
    } 
    Serial.print(data[i],HEX); 
    Serial.print(" "); 
  }
  Serial.println("");
}
