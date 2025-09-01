#include <Servo.h>

const int esc1Pin      = 2;
const int esc2Pin      = 3;

const int powerLedPin  = 8;
const int statusLedPin = 6;

const int buzzerPin    = 7;

const int encoderCLKPin  = 4; //green
const int encoderDataPin  = 5; //yelow

const int encoderSwPin = 9;

const int throttleMin  = 1250;
const int throttleMax  = 1750;
const int throttleNone = 1500;

volatile long encoderCount = 1500;
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

  Serial.begin(9600);

  pinMode(powerLedPin, OUTPUT);
  digitalWrite(powerLedPin, HIGH);
  pinMode(statusLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(encoderSwPin, INPUT_PULLUP);
  pinMode(encoderCLKPin, INPUT_PULLUP);
  pinMode(encoderDataPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderCLKPin), onEncoderAChange, RISING);
  esc1.writeMicroseconds(throttleMin);
  esc2.writeMicroseconds(throttleMin);
}

void loop() {
  noInterrupts();
  long count = encoderCount;
  interrupts();

  // Serial.println(count);

  count = constrain(count, 0, 100);
  int pulse = map(count, 0, 100, throttleMin, throttleMax);

  esc1.writeMicroseconds(pulse);
  esc2.writeMicroseconds(pulse);

  analogWrite(statusLedPin, map(pulse, throttleMin, throttleMax, 0, 255));

  if (pulse >= throttleMax) tone(buzzerPin, 1000);
  else noTone(buzzerPin);

  if (digitalRead(encoderSwPin) == LOW) {
    esc1.writeMicroseconds(throttleNone);
    esc2.writeMicroseconds(throttleNone);
    digitalWrite(powerLedPin, LOW);
    delay(200);
    digitalWrite(powerLedPin, HIGH);
    delay(200);
  }

  delay(10);
}

void onEncoderAChange() {
  int bState = digitalRead(encoderDataPin);
  encoderCount += (bState != lastEncoderB) ? 1 : -1;
  lastEncoderB = bState;
}
