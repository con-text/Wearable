#include <Wire.h>
#include <ADXL345.h>

ADXL345 accel = ADXL345();

const int INTERRUPT_PIN = 2;

void setup() {
  // put your setup code here, to run once
  Wire.begin();
  Serial.begin(9600);
  
  pinMode(INTERRUPT_PIN, INPUT);
    
  while (!accel.begin()) {
      Serial.println("No ADXL345 detected...");
  } 
  
  Serial.println("Found ADXL345 :)"); 
  accel.setupTapInterrupts();
}

void loop() {
  int interruptSource = accel.readInterruptSource();
   // we use a digitalRead instead of attachInterrupt so that we can use delay()
  if(digitalRead(INTERRUPT_PIN)) {
    // Weird case where we're getting all the bits set
    if ((interruptSource != 255) && (interruptSource & B00100000)) {
      Serial.println("Double tap");
      Serial.println(""); // closing Axes line
    }
    //delay(500);
  }
  
  delay(15);
}
