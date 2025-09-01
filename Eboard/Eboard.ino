#include <Servo.h>

#define esc1Pin 4
#define esc2Pin 5

const int powerLedPin  = 8;
const int statusLedPin = 6;

const int buzzerPin    = 7;

#define encoderCLKPin 2 //green
#define encoderDataPin 3 //yelow

#define throttleMin 1250
#define throttleMax 1750
#define throttleNone 1500
#define throttleDeadband 4 // out of 50

volatile int encoderPos = 0;
Servo esc1, esc2;

void interuptHandler(){
  int clkState  = digitalRead(encoderCLKPin);
  int dataState = digitalRead(encoderDataPin);
  if (clkState == dataState) encoderPos++;
  else encoderPos--;
}

void setup() {
  esc1.attach(esc1Pin);
  esc2.attach(esc2Pin);

  //for testing. remove for final code
  esc1.writeMicroseconds(1550);
  esc2.writeMicroseconds(1550);
  delay(2000);
  esc1.writeMicroseconds(1450);
  esc2.writeMicroseconds(1450);
  delay(2000);
  esc1.writeMicroseconds(throttleNone);
  esc2.writeMicroseconds(throttleNone);

  Serial.begin(115200);
  Serial.println("init");

  pinMode(encoderCLKPin, INPUT_PULLUP);
  pinMode(encoderDataPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(encoderCLKPin), interuptHandler, CHANGE);
}


void loop() {
  encoderPos = constrain(encoderPos, -50, 50);
  int pulse = map(encoderPos, -50, 50, throttleMin, throttleMax);

  if(abs(encoderPos) < throttleDeadband){
    pulse = 0;
  }

  esc1.writeMicroseconds(pulse);
  esc2.writeMicroseconds(pulse);

  // analogWrite(statusLedPin, map(pulse, throttleMin, throttleMax, 0, 255));
  // if (pulse >= throttleMax) tone(buzzerPin, 1000);
  // else noTone(buzzerPin);
  // if (digitalRead(encoderSwPin) == LOW) {
  //   esc1.writeMicroseconds(throttleNone);
  //   esc2.writeMicroseconds(throttleNone);
  //   digitalWrite(powerLedPin, LOW);
  //   delay(200);
  //   digitalWrite(powerLedPin, HIGH);
  //   delay(200);
  // }
  Serial.println(encoderPos);
  delay(50);
}


