#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "rf.h"
#include "shared.h"

#define CE_PIN 9
#define CSN_PIN 10

#define SWITCH_PIN 15      // Mode switch input pin (A1)
#define JOYSTICK_PIN 14   // Joystick analog input (A0)
#define DEADBAND 5       // Deadband around neutral (±5 µs out of 1000 µs range)



static RF24 radio(CE_PIN, CSN_PIN);


static bool radioNumber = 0;
bool role = true;  // true = TX role, false = RX role

u16 packetNum = 0;  



Speedmode mode = Speedmode::SLOW; // Start in demo mode
long timestamp;                 // Last time switch was pressed
bool lastSwitch = 1;   // Previous switch state

void rfSetup(){

  printf_begin();
  pinMode(SWITCH_PIN, INPUT_PULLUP);  // Switch is active LOW
  pinMode(JOYSTICK_PIN, INPUT);


  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
 
 
  while (!Serial.available()) {
    // wait for user input
  }

  //set 2 byte CRCs, and 5 byte address
  radio.setCRCLength(RF24_CRC_16);
  radio.setAddressWidth(ADDR_LEN); 
 
  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
 
  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(packet_t));  // float datatype occupies 4 bytes
 
  // set the TX address of the RX node for use on the TX pipe (pipe 0)
  radio.stopListening(addresses[radioNumber]);  // put radio in TX mode
 
  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, addresses[!radioNumber]);  // using pipe 1
 
  // additional setup specific to the node's RX role
  if (!role) {
    radio.startListening();  // put radio in RX mode
  }
  
}

void rfLoop(){


  struct packet_t payload = {};

  payload.seq = packetNum++;
  payload.flags = FLAG_NONE;

  // === Read Inputs ===
  int joystick = analogRead(JOYSTICK_PIN);
  unsigned char switchVal = !digitalRead(SWITCH_PIN);  // Active when pressed

  printf("Joystick: %d, Switch: %d\n", joystick, switchVal);


  // Detect rising edge (button pressed now, not pressed before)
  if (!lastSwitch && switchVal) {
    timestamp = millis();
  }

  // If switch is currently held down...
  if (switchVal) {
    // Held for >5 seconds → toggle mode
    if (millis() - timestamp > 5000) {
      mode = (mode == FAST) ? SLOW : FAST;
      timestamp = millis();

      //trigger mode switch
      payload.flags = (mode == FAST) ? FLAG_FAST : FLAG_SLOW;
    }
  }

  // Update last switch state
  lastSwitch = switchVal;

  int PWMOut = (mode == FAST)
    ? map(joystick, 0, 1023, SPEED_FAST_REV, SPEED_FAST_FWD)
    : map(joystick, 0, 1023, SPEED_SLOW_REV, SPEED_SLOW_FWD);

  // Apply deadband around stopSpeed
  if (abs(PWMOut - SPEED_STOP) < DEADBAND) {
    PWMOut = SPEED_STOP;
  }

  payload.throttle = PWMOut;
 

  unsigned long start_timer = micros();                // start the timer
    bool report = radio.write(&payload, (u8) sizeof(packet_t));  // transmit & save the report
    unsigned long end_timer = micros();                  // end the timer
 
    if (report) {
      Serial.print("Transmission successful! ");  // payload was delivered
      printf("Time to transmit = %lu us, ", end_timer - start_timer);
      printf("seq=%d, throttle = %d, flags = %d\n",payload.seq, payload.throttle, payload.flags);
    } else {
      Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    }
}