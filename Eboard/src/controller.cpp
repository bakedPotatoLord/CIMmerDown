#include <SPI.h>
#include "RF24.h"
#include "controller.h"
#include "shared.h"
#include <avr/sleep.h>

#define CE_PIN 9
#define CSN_PIN 10


#define SWITCH_PIN (A2+0U)   //16 Mode switch input pin (A2)
#define JOYSTICK_PIN (A1+0U) //15   // Joystick analog input (A1)  
#define LED_PIN 7
#define DEADBAND 5       // Deadband around neutral (±5 µs out of 1000 µs range)

#define BATT_LOW 3.4f
#define BATT_DEAD 3.2f

#define VDIV_PIN 14 // Voltage divider pin A0

#define VDIV_R1 33e3f // resistor from A0 to GND 
#define VDIV_R2 100e3f // resistor from VCC RAW to A0 
#define VDIV_REF 3.3f // reference voltage 
//convert raw ADC value to input voltage
#define CONVERT_VDIV(raw) (raw * (VDIV_REF / 1023.0f) ) * (VDIV_R1 + VDIV_R2) / VDIV_R1

static RF24 radio(CE_PIN, CSN_PIN);

static bool radioNumber = 0;
bool role = true;  // true = TX role, false = RX role

u16 packetNum = 0;  


Speedmode mode = Speedmode::SLOW; // Start in demo mode
long timestamp; // Last time switch was pressed
bool lastSwitch = 1; // Previous switch state
Status status = Status::ok_slow;

void controllerSetup(){
  pinMode(SWITCH_PIN, INPUT_PULLUP);  // Switch is active LOW
  pinMode(JOYSTICK_PIN, INPUT);
  pinMode(VDIV_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  if (!radio.begin()) {
    #ifdef _debug 
    Serial.println(F("radio hardware is not responding!!"));
    #endif
    status = Status::hardwareFault;
    return;
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

void controllerLoop(){
  SetLED();

  if(status == Status::hardwareFault){
    return;
  }else if (status == ctrl_batt_dead){
    //disable peripherals
    radio.powerDown();
    // put controller into sleep mode
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
  }

  struct packet_t payload = {};
  payload.seq = packetNum++;
  payload.flags = FLAG_NONE;

  // === Read Inputs ===
  u16 joystick = analogRead(JOYSTICK_PIN);
  float battery = CONVERT_VDIV( (float) analogRead(VDIV_PIN));
  unsigned char switchVal = !digitalRead(SWITCH_PIN);  // Active when pressed

  if(battery <= BATT_LOW) status = Status::ctrl_batt_low;
  if(battery <= BATT_DEAD){
    status = Status::ctrl_batt_dead;
    return;
  } 

  #ifdef _debug
  Serial.print("Joystick: ");
  Serial.print(joystick);
  Serial.print(", Switch: ");
  Serial.println(switchVal);
  Serial.print("Battery: ");
  Serial.println(battery);
  #endif

  // Detect rising edge (button pressed now, not pressed before)
  if (!lastSwitch && switchVal) {
    timestamp = millis();
  }

  // If switch is currently held down...
  if (switchVal) {
    // Held for >5 seconds → toggle mode
    if (millis() - timestamp > 2000) {
      mode = (mode == FAST) ? SLOW : FAST;
      timestamp = millis();

      //trigger mode switch
      payload.flags = (mode == FAST) ? FLAG_FAST : FLAG_SLOW;
      if(status == Status::ok_fast || status == Status::ok_slow){
        status = (mode == FAST) ? Status::ok_fast : Status::ok_slow;
      }
    }
  }

  // Update last switch state
  lastSwitch = switchVal;

  //map joystick val to pwm out
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

        if(received.flags & FLAG_BOARD_BATT_LOW){
          status = Status::board_batt_low;
        }else if(received.flags & FLAG_BOARD_BATT_DEAD){
          status = Status::board_batt_dead;
        }
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


//should be called periodically. Checks status and sets LED
void SetLED(){
  u32 ms = millis();
  if(status == Status::hardwareFault){
    //one blink every 3 seconds
    ms %= 3000;
    digitalWrite(LED_PIN, ms < 500);

  }else if(status == Status::await_connect){
    //2groups of 2 fast blinks
    ms %=1500;
    digitalWrite(LED_PIN, ms < 500 && ms%250<125);
  }else if(status == Status::ok_slow){
    //slow blink
    ms %= 1500;
    digitalWrite(LED_PIN, ms < 750);
  }else if(status == Status::ok_fast){
    //fast blink
    ms %= 750;
    digitalWrite(LED_PIN, ms < 375);
  }else if(status == Status::e_stop){
    //solid light
    digitalWrite(LED_PIN, 1);
  }else if(status == Status::board_batt_low){
    //2 fast blinks at start
    ms %= 3000;
    digitalWrite(LED_PIN, ms < 500 && ms%250<125);
  }else if(status == Status::board_batt_dead){
    //3 fast blinks at start
    ms %= 3000;
    digitalWrite(LED_PIN, ms < 750 && ms%250<125);
  }else if(status == Status::ctrl_batt_low){
    //2 slower blinks at start
    ms %= 3000;
    digitalWrite(LED_PIN, ms < 1000 && ms%500<250);
  } else if(status == Status::ctrl_batt_dead){
    //nothing
    digitalWrite(LED_PIN, 0);
  }else{
    digitalWrite(LED_PIN, LOW);
  }
}
