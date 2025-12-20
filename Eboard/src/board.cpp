#include <Servo.h>
#include "RF24.h"
#include <Arduino.h>
#include "board.h"
#include "shared.h"

// === Pin Definitions ===
#define ESC1Pin 5        // ESC #1 control signal pin
#define ESC2Pin 6        // ESC #2 control signal pin
#define BUZZER_PIN 3      // Buzzer output pin
#define VDIV_PIN 14 // Voltage divider pin A0

#define VDIV_R1 10e3f // resistor from A0 to GND
#define VDIV_R2 20e3f // resistor from VCC RAW to A0
#define VDIV_REF 4.97f // reference voltage

//convert raw ADC value to input voltage
#define CONVERT_VDIV(raw) (raw * (VDIV_REF / 1023.0f) ) * (VDIV_R1 + VDIV_R2) / VDIV_R1

#define CE_PIN 9
#define CSN_PIN 10

// === Global Objects ===
Servo esc1, esc2;
static RF24 radio(CE_PIN, CSN_PIN);

// === Global State Variables ===
static bool estop = 0;        // Emergency stop flag
static bool foundController = 0;
static bool radioNumber = 1;
const bool role = false;  // true = TX role, false = RX role
static u32 lastMsg;
static u8 batteryState = FLAG_BATT_OK;

void boardSetup(){

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    #ifdef _debug
    Serial.println("radio hardware is not responding!!");
    #endif
    while (1) {}  // hold in infinite loop
  }

  Serial.begin(115200);
  esc1.attach(ESC1Pin);
  esc2.attach(ESC2Pin);

  pinMode(VDIV_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  } 
 
  
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  radio.enableDynamicAck();  // ACK payloads are dynamically sized
  radio.setPayloadSize(sizeof(ack_payload_t));
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

  pinMode(BUZZER_PIN, OUTPUT);
  // Startup tones for power-on confirmation
  #ifndef mute
  tone(BUZZER_PIN, 512, 100);
  delay(100);
  tone(BUZZER_PIN, 1024, 100);
  #endif
}

void boardLoop(){
  // === Handle Emergency Stop ===
  if (estop) {
    esc1.writeMicroseconds(SPEED_STOP);
    esc2.writeMicroseconds(SPEED_STOP);

    #ifdef _debug
    Serial.println("Emergency Stop!");
    #endif
    // Audible alarm: 3 short alternating beeps
    #ifndef mute
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1024, 200);
      delay(100);
      tone(BUZZER_PIN, 512, 200);
      delay(100);
    }
    #endif
    return;  // Skip rest of loop
  }

  

  packet_t payload;
  ack_payload_t ackPayload;
  uint8_t pipe;
  if (radio.available(&pipe)) { // is there a payload? get the pipe number that received it
    foundController = 1;
    uint8_t bytes = radio.getDynamicPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes); // fetch payload from FIFO
    lastMsg = millis();

    
    float battVoltage = CONVERT_VDIV(analogRead(VDIV_PIN));
    if (battVoltage < 9.6f){
      batteryState = FLAG_BATT_NEAR_LOW;
    }else if(battVoltage < 9.0f){
      batteryState = FLAG_BATT_LOW;
    }
    #ifdef _debug
    Serial.print("Board recieved Packet.Sequence number: ");
    Serial.println(payload.seq);
    Serial.print("timestamp:");
    Serial.println(lastMsg);
    Serial.print("Batt Voltage: "); 
    Serial.println(battVoltage);
    Serial.print("Battery flags: "); 
    Serial.println(batteryState);
    Serial.println();
    #endif
  
    radio.writeAckPayload(1, &ackPayload, sizeof(ackPayload));
    if(batteryState != FLAG_BATT_LOW){
      u16 speed = payload.throttle;
      u16 invertedSpeed = map(speed, 1000,2000,2000,1000);
      esc1.writeMicroseconds(speed);
      esc2.writeMicroseconds(invertedSpeed);
    }
  }else{
    //no packet available

    u32 m = millis();
    if(foundController && (m - lastMsg > 3000)){
      //if had controller, and no message for 1 second
      #ifdef _debug
      Serial.println("Controller lost");
      Serial.println(m);
      Serial.println(lastMsg);
      #endif
      estop = 1;
    }
  }
}