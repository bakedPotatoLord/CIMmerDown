#include <SPI.h>
#include "RF24.h"
#include "controller.h"
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
long timestamp; // Last time switch was pressed
bool lastSwitch = 1; // Previous switch state

void rfSetup(){
  pinMode(SWITCH_PIN, INPUT_PULLUP);  // Switch is active LOW
  pinMode(JOYSTICK_PIN, INPUT);

  if (!radio.begin()) {
    #ifdef _debug 
    Serial.println(F("radio hardware is not responding!!"));
    #endif
    while (1) {}  // hold in infinite loop
  }

  //set 2 byte CRCs, and 5 byte address
  radio.setCRCLength(RF24_CRC_16);
  radio.setAddressWidth(ADDR_LEN); 
 
  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
 
  radio.enableDynamicAck();  // ACK payloads are dynamically sized
  radio.setPayloadSize(sizeof(packet_t));

  // Acknowledgement packets have no payloads by default. We need to enable
  // this feature for all nodes (TX & RX) to use ACK payloads.
  radio.enableAckPayload();
  radio.setRetries(5, 15);
 
 
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

  #ifdef _debug
  Serial.print("Joystick: ");
  Serial.print(joystick);
  Serial.print(", Switch: ");
  Serial.println(switchVal);
  #endif

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
      #ifdef _debug
      Serial.println("Transmission successful! ");  // payload was delivered
      Serial.print("Time to transmit = ");
      Serial.print(end_timer - start_timer);
      Serial.print(" us, seq=");
      Serial.println(payload.seq);
      Serial.print(", throttle = ");
      Serial.print(payload.throttle);
      Serial.print(", flags = ");
      Serial.println(payload.flags);
      #endif

      u8 pipe;
      if (radio.available(&pipe)) {  // is there an ACK payload? grab the pipe number that received it
        ack_payload_t received;
        radio.read(&received, sizeof(received));  // get incoming ACK payload
          #ifdef _debug        
          Serial.print("Received ACK packet, battery = ");
          Serial.println(received.flags);
          Serial.println();
          #endif
      } else {
        #ifdef _debug
        Serial.println(" no incoming ACK packet\n");  // empty ACK packet received
        #endif
      }

    } else {
      #ifdef _debug
      Serial.println("Transmission failed or timed out");  // payload was not delivered
      #endif
    }
}