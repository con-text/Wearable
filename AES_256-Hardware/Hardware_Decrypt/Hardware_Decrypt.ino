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
      
  uint16_t address = 0xF010;
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 16] = {0};
  aes132m_random(rxBuffer);
  //aes132m_block_read(address, 2, rxBuffer);
    
  delay(100);
 // uint16_t address = 0xF010;
  //aes132c_wri(address, 2, rx_buffer);
  
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  } 
 
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

