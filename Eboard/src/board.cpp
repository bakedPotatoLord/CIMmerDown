#include <Servo.h>
#include "RF24.h"
#include <Arduino.h>
#include "board.h"
#include "shared.h"

// === Pin Definitions ===
#define ESC1Pin 5        // ESC #1 control signal pin
#define ESC2Pin 6        // ESC #2 control signal pin
#define buzzerPin 3      // Buzzer output pin

#define CE_PIN 9
#define CSN_PIN 10


// === Global Objects ===
Servo esc1, esc2;
static RF24 radio(CE_PIN, CSN_PIN);

// === Global State Variables ===
static bool estop = 0;        // Emergency stop flag

static bool radioNumber = 0;
const bool role = false;  // true = TX role, false = RX role


void boardSetup(){

  
 
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  Serial.begin(115200);
  esc1.attach(ESC1Pin);
  esc2.attach(ESC2Pin);

  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
 
  // print example's introductory prompt
  Serial.println(F("RF24/examples/GettingStarted"));
 
  // To set the radioNumber via the Serial monitor on startup
  Serial.println(F("Which radio is this? Enter '0' or '1'. Defaults to '0'"));
  while (!Serial.available()) {
    // wait for user input
  }
  char input = Serial.parseInt();
  radioNumber = input == 1;
  Serial.print(F("radioNumber = "));
  Serial.println((int)radioNumber);
 
  // role variable is hardcoded to RX behavior, inform the user of this
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
 
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


  pinMode(buzzerPin, OUTPUT);

  // Startup tones for power-on confirmation
  tone(buzzerPin, 512, 200);
  delay(100);
  tone(buzzerPin, 1024, 200);
}

void boardLoop(){
  
 

  // === Handle Emergency Stop ===
  if (estop) {
    esc1.writeMicroseconds(SPEED_STOP);
    esc2.writeMicroseconds(SPEED_STOP);

    // Audible alarm: 3 short alternating beeps
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1024, 200);
      delay(100);
      tone(buzzerPin, 512, 200);
      delay(100);
    }
    return;  // Skip rest of loop
  }

  packet_t payload;

  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that received it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes);             // fetch payload from FIFO
    Serial.print(F("Received "));
    Serial.print(bytes);  // print the size of the payload
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);  // print the pipe number
    Serial.print(F(": "));
    Serial.println(payload.seq);  // print the payload's value
  }

  // === Map Joystick Input to ESC PWM ===
  
  // === Drive ESCs ===
  // esc1.writeMicroseconds(PWMOut);
  // esc2.writeMicroseconds(invertedPWMOut);


  delay(50);  // Small loop delay for stability  
}