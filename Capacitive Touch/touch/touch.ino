const int touchPin = 2;

#include <Timer.h>

Timer t;

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(9600);

  pinMode(touchPin, INPUT);
  
  t.every(1000, readCapacitiveTouch);
}

void readCapacitiveTouch()
{
  // put your main code here, to run repeatedly:
  int buttonState = digitalRead(touchPin);
  
  if (buttonState == HIGH) {
    Serial.println("Touch");
  }
}

void loop() {
   t.update();
}
