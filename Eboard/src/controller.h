
#include <Arduino.h>
#include "shared.h"

enum Status {
  hardwareFault,
  await_connect,
  ok_slow,
  ok_fast,
  e_stop, //solid light
  board_batt_low,
  board_batt_dead,
  ctrl_batt_low,
  ctrl_batt_dead //immediately puts controller into sleep mode
};

void controllerSetup();

void controllerLoop();

void SetLED();