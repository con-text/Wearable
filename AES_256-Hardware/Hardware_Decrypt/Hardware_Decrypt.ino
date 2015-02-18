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
  
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer2);
  
  for (int i = 0; i < sizeof(rxBuffer2); i++) {
    Serial.println(rxBuffer2[i], HEX);
  }
  
  uint8_t dataToDecrypt[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    
   uint8_t inMac[] = {
    0xAA, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB };
    
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN+16] = {0};
  decrypt(dataToDecrypt, inMac, rxBuffer);
    
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  }
  
      
 /* uint8_t keyOne[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    
    
    
  writeKey(keyOne, 0);*/
 // uint16_t address = 0xF200;
 // uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 16] = {0};
  //aes132m_block_read(address, 16, rxBuffer);
  
  /*
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN] = {0};
  nonce(rxBuffer);
  delay(100);
  
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  }
  
  uint8_t rxBuffer2[AES132_RESPONSE_SIZE_MIN + 32] = {0};
  encrypt(rxBuffer2);
  
  delay(500);
  
  for (int i = 0; i < sizeof(rxBuffer2); i++) {
    Serial.println(rxBuffer2[i], HEX);
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

