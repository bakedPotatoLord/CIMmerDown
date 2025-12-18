#include "board.h"
#include "rf.h"
#include "Arduino_BuiltIn.h"

#define remote


// ==========================================================
// setup() — Runs once on startup
// ==========================================================
void setup() {
  Serial.begin(115200);

  #ifdef remote
    rfSetup();  // Initialize radio for remote control
  #else
    boardSetup();
  #endif
}

// ==========================================================
// loop() — Main control loop, runs continuously
// ==========================================================
void loop() {
  #ifdef remote
    rfSetup();  // Initialize radio for remote control
  #else
    boardLoop();
  #endif
}
