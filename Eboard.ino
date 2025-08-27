#include <Servo.h>

const int esc1Pin      = 3;
const int esc2Pin      = 5;
const int powerLedPin  = 8;
const int statusLedPin = 6;
const int buzzerPin    = 7;
const int encoderAPin  = 2;
const int encoderBPin  = 4;
const int encoderSwPin = 9;

const int throttleMin  = 1000;
const int throttleMax  = 2000;

volatile long encoderCount = 0;
int lastEncoderB = LOW;
Servo esc1, esc2;

void setup() {
  esc1.attach(esc1Pin);
  esc2.attach(esc2Pin);

  esc1.writeMicroseconds(throttleMin);
  esc2.writeMicroseconds(throttleMin);
  delay(2000);
  esc1.writeMicroseconds(throttleMax);
  esc2.writeMicroseconds(throttleMax);
  delay(2000);
  esc1.writeMicroseconds(throttleMin);
  esc2.writeMicroseconds(throttleMin);
  delay(2000);

  pinMode(powerLedPin, OUTPUT);
  digitalWrite(powerLedPin, HIGH);
  pinMode(statusLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(encoderSwPin, INPUT_PULLUP);
  pinMode(encoderAPin, INPUT_PULLUP);
  pinMode(encoderBPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderAPin), onEncoderAChange, RISING);
  esc1.writeMicroseconds(throttleMin);
  esc2.writeMicroseconds(throttleMin);
}

void loop() {
  noInterrupts();
  long count = encoderCount;
  interrupts();

  count = constrain(count, 0, 100);
  int pulse = map(count, 0, 100, throttleMin, throttleMax);

  esc1.writeMicroseconds(pulse);
  esc2.writeMicroseconds(pulse);

  analogWrite(statusLedPin, map(pulse, throttleMin, throttleMax, 0, 255));

  if (pulse >= throttleMax) tone(buzzerPin, 1000);
  else noTone(buzzerPin);

  if (digitalRead(encoderSwPin) == LOW) {
    esc1.writeMicroseconds(throttleMin);
    esc2.writeMicroseconds(throttleMin);
    digitalWrite(powerLedPin, LOW);
    delay(200);
    digitalWrite(powerLedPin, HIGH);
    delay(200);
  }

  delay(10);
}

void onEncoderAChange() {
  int bState = digitalRead(encoderBPin);
  encoderCount += (bState != lastEncoderB) ? 1 : -1;
  lastEncoderB = bState;
}
