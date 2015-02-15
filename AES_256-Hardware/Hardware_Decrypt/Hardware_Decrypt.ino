#include <Wire.h>
#include <aes132.h>
//#include <aes132_i2c.h>
#include <aes132_commands.h>

byte val = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting I2C");
  Wire.begin();
  
  uint8_t sla = 0x50;  
  uint8_t statr[2] = {0xFF, 0xF0};
  
  Wire.beginTransmission(sla);
  Wire.write(statr, (uint8_t)2);
  int ret = Wire.endTransmission();
  Serial.println(ret);

  int available_bytes = Wire.requestFrom((int)sla, 1);
  Serial.println(available_bytes);

  //i2c_enable(); 
  //getInfo();
}

void loop() {
  // put your main code here, to run repeatedly:

}

byte calculateTemperature()
{
  uint16_t response[1] = {0};
  
  uint8_t ret_code = aes132m_temp_sense(response);
  
  for (int i = 0; i < sizeof(response); i++) {
    Serial.print(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}

byte getInfo()
{
  uint8_t response[1] = {0};
  
  uint8_t ret_code = aes132m_info(6, response);
  
  for (int i = 0; i < sizeof(response); i++) {
    Serial.print(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}
