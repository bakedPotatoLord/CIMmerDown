#ifdef REMOTE
  #include "rf.h"
#else
  #include "board.h"
#endif

#include "Arduino.h"
#include <avr/sleep.h>

#define CYCLE_FREQ 20UL //clock frequency in Hz
#define CYCLE_WAVELEN ( 1000UL / CYCLE_FREQ) //wavelength in ms

//if REMOTE is not defined, assume reciever


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

u32 lastLoop = 0;

void loop() {
  u32 now = millis();

  if(now - lastLoop > CYCLE_WAVELEN){ 
    Serial.println(now);
    Serial.println(lastLoop);
    lastLoop = millis();
    #ifdef REMOTE
      rfLoop();  // Initialize radio for remote control
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
