#ifdef REMOTE
  #include "rf.h"
#else
  #include "board.h"
#endif

#include "Arduino.h"

//if REMOTE is not defined, assume reciever

bool led =0;

// ==========================================================
// setup() — Runs once on startup
// ==========================================================
void setup() {
  Serial.begin(115200);

  #ifdef REMOTE
    rfSetup();  // Initialize radio for remote control
  #else
    boardSetup();
  #endif

  pinMode(LED_BUILTIN, OUTPUT);
}

// ==========================================================
// loop() — Main control loop, runs continuously
// ==========================================================
void loop() {
  #ifdef REMOTE
    rfLoop();  // Initialize radio for remote control
  #else
    boardLoop();
  #endif

  led = !led;
  digitalWrite(LED_BUILTIN, led);
  // Serial.println("hello from the loop");
  delay(1000);

}
