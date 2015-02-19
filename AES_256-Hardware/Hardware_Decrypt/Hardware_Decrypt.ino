#include <Wire.h>
#include <aes132.h>
//#include <aes132_i2c.h>
#include <aes132_commands.h>

byte val = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  Wire.begin();
  
  
  /* UNCOMMENT FOR DECRYPTION */
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN] = {0};
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
  decrypt(dataToDecrypt, inMac, rxBuffer);
    
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  }
   
      
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

byte calculateTemperature()
{
  uint16_t response[1] = {0};
  
  uint8_t ret_code = aes132m_temp_sense(response);
  Serial.println("Response bytes:");
  for (int i = 0; i < sizeof(response); i++) {
    Serial.println(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}

byte getInfo()
{
  uint8_t response[2] = {0,0};
  
  uint8_t ret_code = aes132m_info(0x06, response);
  Serial.println("Response bytes:");
  for (int i = 0; i < sizeof(response); i++) {
    Serial.println(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}

