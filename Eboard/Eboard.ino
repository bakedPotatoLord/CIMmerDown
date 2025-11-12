#include <Servo.h>

// === Pin Definitions ===
#define joystickPin 14   // Joystick analog input (A0)
#define ESC1Pin 5        // ESC #1 control signal pin
#define ESC2Pin 6        // ESC #2 control signal pin
#define buzzerPin 3      // Buzzer output pin
#define switchPin 8      // Mode switch input pin

// === Speed Limits ===
#define normSpeedFWD 2000
#define normSpeedREV 1000
#define demoSpeedFWD 1750
#define demoSpeedREV 1250
#define stopSpeed 1500   // Neutral pulse width (ESC stop position)

// === Other Config ===
#define deadband 5       // Deadband around neutral (±5 µs out of 1000 µs range)

// === Global Objects ===
Servo esc1, esc2;

// === Mode Enumeration ===
enum Speedmode {
  NORM,
  DEMO,
};

// === Global State Variables ===
unsigned char estop = 0;        // Emergency stop flag
Speedmode mode = Speedmode::DEMO; // Start in demo mode

long timestamp;                 // Last time switch was pressed
unsigned char lastSwitch = 1;   // Previous switch state

// ==========================================================
// setup() — Runs once on startup
// ==========================================================
void setup() {
  Serial.begin(115200);

  esc1.attach(ESC1Pin);
  esc2.attach(ESC2Pin);

  pinMode(switchPin, INPUT_PULLUP);  // Switch is active LOW
  pinMode(buzzerPin, OUTPUT);
  pinMode(joystickPin, INPUT);

  // Startup tones for power-on confirmation
  tone(buzzerPin, 512, 200);
  delay(100);
  tone(buzzerPin, 1024, 200);
}

// ==========================================================
// loop() — Main control loop, runs continuously
// ==========================================================
void loop() {
  // === Read Inputs ===
  int joystick = analogRead(joystickPin);
  unsigned char switchVal = !digitalRead(switchPin);  // Active when pressed

  // === Check for Joystick Disconnect ===
  // Temporarily drive the joystick pin HIGH and re-read to detect open circuit.
  pinMode(joystickPin, OUTPUT);
  digitalWrite(joystickPin, HIGH);
  delay(2);
  pinMode(joystickPin, INPUT);
  int val = analogRead(joystickPin);

  Serial.println(val);
  if (val > 1020) {
    estop = 1;  // Treat near-5V reading as disconnect
  }

  // === Handle Emergency Stop ===
  if (estop) {
    esc1.writeMicroseconds(stopSpeed);
    esc2.writeMicroseconds(stopSpeed);

    // Audible alarm: 3 short alternating beeps
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1024, 200);
      delay(100);
      tone(buzzerPin, 512, 200);
      delay(100);
    }
    return;  // Skip rest of loop
  }

  // === Handle Mode Switch ===

  // Detect rising edge (button pressed now, not pressed before)
  if (!lastSwitch && switchVal) {
    timestamp = millis();
  }

  // If switch is currently held down...
  if (switchVal) {
    // Held for >5 seconds → toggle mode
    if (millis() - timestamp > 5000) {
      mode = (mode == NORM) ? DEMO : NORM;
      timestamp = millis();

      // Play triple ascending tone sequence to indicate mode change
      for (int i = 0; i < 3; i++) {
        tone(buzzerPin, 512, 200);
        delay(100);
        tone(buzzerPin, 1024, 200);
        delay(100);
        tone(buzzerPin, 2048, 200);
        delay(100);
      }
    }
  }

  // Update last switch state
  lastSwitch = switchVal;

  // === Map Joystick Input to ESC PWM ===
  int PWMOut = (mode == NORM)
                 ? map(joystick, 676, 0, normSpeedFWD, normSpeedREV)
                 : map(joystick, 676, 0, demoSpeedFWD, demoSpeedREV);

  // Apply deadband around stopSpeed
  if (abs(PWMOut - stopSpeed) < deadband) {
    PWMOut = stopSpeed;
  }

  // Invert second ESC output for mirrored motor orientation
  int invertedPWMOut = map(PWMOut, 1000, 2000, 2000, 1000);

  // === Drive ESCs ===
  esc1.writeMicroseconds(PWMOut);
  esc2.writeMicroseconds(invertedPWMOut);

  // === Telemetry Output ===
  Serial.print("joystick:");
  Serial.print(joystick);
  Serial.print(", Switch:");
  Serial.print(switchVal);
  Serial.print(", PWM out:");
  Serial.print(PWMOut);
  Serial.print(", mode:");
  Serial.print(mode);
  Serial.print("\n");

  delay(50);  // Small loop delay for stability
}
