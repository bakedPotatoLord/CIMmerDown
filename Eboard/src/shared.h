#ifndef SHARED_H
#define SHARED_H

#include <Arduino.h>


// === Speed Limits ===
#define normSpeedFWD 2000
#define normSpeedREV 1000
#define demoSpeedFWD 1750
#define demoSpeedREV 1250
#define stopSpeed 1500   // Neutral pulse width (ESC stop position)

#define ADDR_LEN 5


struct packet_t {
  u16 seq;
  u16 throttle; // 0-200
  u8 flags; //0x01 = switch to fast mode. 0x02 = switch to slow mode
};  

const u8 addresses[2][ADDR_LEN+1] = { "TRSMT", "RCVER" };






#endif