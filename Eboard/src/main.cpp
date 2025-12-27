#ifdef REMOTE
  #include "controller.h"
#else
  #include "board.h"
#endif

#include "Arduino.h"
#include <avr/sleep.h>

#define CYCLE_FREQ 1UL //clock frequency in Hz
#define CYCLE_WAVELEN ( 1000UL / CYCLE_FREQ) //wavelength in ms

//if REMOTE is not defined, assume reciever


// ==========================================================
// setup() — Runs once on startup
// ==========================================================
void setup() {
  #ifdef _debug
  Serial.begin(115200);
  #endif

  #ifdef REMOTE
    controllerSetup();  // Initialize radio for remote control
  #else
    boardSetup();
  #endif

  #ifdef _debug
  Serial.println("debug");
  #else
  Serial.println("no debug");
  #endif


}

// ==========================================================
// loop() — Main control loop, runs continuously
// ==========================================================

u32 lastLoop = 0;

void loop() {
  u32 now = millis();
  if(now - lastLoop > CYCLE_WAVELEN){ 
    #ifdef _debug
    Serial.print("\nnow: ");
    Serial.println(now);
    #endif
    lastLoop = millis();
    #ifdef REMOTE
      controllerLoop();  // Initialize radio for remote control
    #else
      boardLoop();
    #endif
  }else{
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
  }
}
