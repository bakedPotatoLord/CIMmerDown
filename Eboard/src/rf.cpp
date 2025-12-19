#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "rf.h"
#include "shared.h"

#define CE_PIN 9
#define CSN_PIN 10

#define switchPin 8      // Mode switch input pin
#define joystickPin 14   // Joystick analog input (A0)
#define deadband 5       // Deadband around neutral (±5 µs out of 1000 µs range)

// === Speed Limits ===
#define normSpeedFWD 2000
#define normSpeedREV 1000
#define demoSpeedFWD 1750
#define demoSpeedREV 1250
#define stopSpeed 1500   // Neutral pulse width (ESC stop position)


static RF24 radio(CE_PIN, CSN_PIN);


static bool radioNumber = 0;
bool role = true;  // true = TX role, false = RX role

u16 packetNum = 0;  

enum Speedmode {
  NORM,
  DEMO,
};

Speedmode mode = Speedmode::DEMO; // Start in demo mode
long timestamp;                 // Last time switch was pressed
bool lastSwitch = 1;   // Previous switch state

void rfSetup(){


  pinMode(switchPin, INPUT_PULLUP);  // Switch is active LOW
  pinMode(joystickPin, INPUT);

  


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

  // === Read Inputs ===
  int joystick = analogRead(joystickPin);
  unsigned char switchVal = !digitalRead(switchPin);  // Active when pressed

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

      //trigger mode switch
    }
  }

  // Update last switch state
  lastSwitch = switchVal;

  int PWMOut = (mode == NORM)
                 ? map(joystick, 676, 0, normSpeedFWD, normSpeedREV)
                 : map(joystick, 676, 0, demoSpeedFWD, demoSpeedREV);

  // Apply deadband around stopSpeed
  if (abs(PWMOut - stopSpeed) < deadband) {
    PWMOut = stopSpeed;
  }

  // Invert second ESC output for mirrored motor orientation
  int invertedPWMOut = map(PWMOut, 1000, 2000, 2000, 1000);


  struct packet_t payload = {
    .seq = packetNum++,
    .throttle = 0
  };
  

  unsigned long start_timer = micros();                // start the timer
    bool report = radio.write(&payload, (u8) sizeof(packet_t));  // transmit & save the report
    unsigned long end_timer = micros();                  // end the timer
 
    if (report) {
      Serial.print(F("Transmission successful! "));  // payload was delivered
      Serial.print(F("Time to transmit = "));
      Serial.print(end_timer - start_timer);  // print the timer result
      Serial.print(F(" us. Sent: "));
      Serial.println(payload.throttle);  // print payload sent
    } else {
      Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    }
}