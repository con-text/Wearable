#include <aes132.h>

#include <Wire.h>
#include <aes132_i2c.h>
#include <aes132_commands.h>

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting I2C");
  Wire.begin();
  i2c_enable(); 
  calculateTemperature();
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
