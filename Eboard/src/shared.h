#ifndef SHARED_H
#define SHARED_H

#include <Arduino.h>


// === Speed Limits ===
#define SPEED_FAST_FWD 2000
#define SPEED_FAST_REV 1000
#define SPEED_SLOW_FWD 1750
#define SPEED_SLOW_REV 1200  
#define SPEED_STOP 1500   // Neutral pulse width (ESC stop position)

#define ADDR_LEN 5

#define FLAG_FAST (1 << 0)
#define FLAG_SLOW (1 << 1)

#define FLAG_NONE 0

enum Speedmode {
  FAST,
  SLOW,
};

struct packet_t {
  u16 seq;
  u16 throttle; // 0-200
  u8 flags; //0x01 = switch to fast mode. 0x02 = switch to slow mode
};  


const u8 addresses[2][ADDR_LEN+1] = { "TRSMT", "RCVER" };






#endif