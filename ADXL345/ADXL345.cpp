// ADXL345 accelerometer  library
// By Denis Ogun

#include <Arduino.h>
#include <ADXL345.h>
#include <Wire.h>

// Chip Communications and Init

bool ADXL345::begin()
{
  /* Check connection */
  uint8_t deviceid = getDeviceID();
  if (deviceid != 0xE5)
  {
    /* No ADXL345 detected ... return false */
    Serial.println(deviceid, HEX);
    return false;
  }
  
  // Enable measurements
  writeRegister(ADXL345_REG_POWER_CTL, 0x08);  
    
  return true;
}

void ADXL345::writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write((uint8_t)reg);
  Wire.write((uint8_t)(value));
  Wire.endTransmission();
}

uint8_t ADXL345::readRegister(uint8_t reg)
{
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  Wire.requestFrom(ADXL345_ADDRESS, 1);
  return (Wire.read());
}

// Functions

uint8_t ADXL345::getDeviceID()
{
  return readRegister(ADXL345_REG_DEVID);
}

void ADXL345::setupTapInterrupts()
{
  // Setup tap duration
  writeRegister(ADXL345_REG_DUR, 0x30);
  // Setup tap threshold
  writeRegister(ADXL345_REG_THRESH_TAP, 0x40);
  // Setup tap axes
  writeRegister(ADXL345_REG_TAP_AXES, B00000001);

  // Setup double tap
  writeRegister(ADXL345_REG_LATENT, 0x40);
  // Setup double tap window
  writeRegister(ADXL345_REG_WINDOW, 0xFF);

  // Setup power control
  writeRegister(ADXL345_REG_POWER_CTL, B00001001);
  // Setup data format
  writeRegister(ADXL345_REG_DATA_FORMAT, B00001010);

  // Setup the interrupt pin to be 0
  writeRegister(ADXL345_REG_INT_MAP, 0);
  // Enable single and double tap interrupt
  writeRegister(ADXL345_REG_INT_ENABLE, B00100000);
}

uint8_t ADXL345::readInterruptSource()
{
  return readRegister(ADXL345_REG_INT_SOURCE);
}
