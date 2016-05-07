// built w/1.6.5 version of Arduino IDE
#include <IRLib.h> // https://learn.adafruit.com/using-an-infrared-library/overview
#include <IRLibMatch.h>
#include <IRLibRData.h>
#include <IRLibTimer.h>

const int IR_PIN = 2;
const long FORWARD = 0xFF18E7;
const long STOP = 0xFF38C7;
const long BACKWARD = 0xFF4AB5;
const long RIGHT = 0xFF5AA5;
const long LEFT = 0xFF10EF;
const long REMOTETOGGLE = 0xFFE21D;

const int AIA = 9;  //(pwm) pin 9 connected to pin A-IA
const int AIB = 5;  //(pwm) pin 5 connected to pin A-IB
const int BIA = 10; //(pwm) pin 10 connected to pin B-IA
const int BIB = 6;  //(pwm) pin 6 connected to pin B-IB

const int LEFT_BUMPER = 4;
const int RIGHT_BUMPER = 3;
const int PING = 11;
const int ECHO = 13;
const int LED_EDGE_DETECT_1 = 7;
const int LED_EDGE_DETECT_2 = 8;

// Global vars
byte speed = 200;  // change this (0-255) to control the speed of the motors
byte halfspeed = 200 ;

long pingTimer = 0;
long ldrTimer = 0;
long cm = 25;
long ldrNoLedEmissions = 0;
int ldrInterval = 150;
boolean usingRemote = false;

IRrecv My_Receiver(IR_PIN); // Create a receiver object to listen
IRdecode My_Decoder;        // Create a decoder object

void setup() {
  // h bridge pins
  pinMode(AIA, OUTPUT); // set pins to output
  pinMode(AIB, OUTPUT);
  pinMode(BIA, OUTPUT);
  pinMode(BIB, OUTPUT);
  // switch bumpers
  pinMode(LEFT_BUMPER, INPUT_PULLUP);
  pinMode(RIGHT_BUMPER, INPUT_PULLUP);
  // ultrasonic ping sensor pins
  pinMode(PING, OUTPUT);
  pinMode(ECHO, INPUT);
  // edge detector setup
  pinMode(LED_EDGE_DETECT_1, OUTPUT);
  pinMode(LED_EDGE_DETECT_2, OUTPUT);
  Serial.begin(115200);
  My_Receiver.enableIRIn(); // Start the receiver
}

void loop() {
  // remoteReact(); // react to remote without using a switch case <---!!! uncomment - comment out 68 to 110 - to test
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();
    My_Decoder.DumpResults();
     
    if (My_Decoder.decode_type== NEC){
      switch(My_Decoder.value) {
        case FORWARD:  //Volume Down
          if(usingRemote == true){
            forward();
            Serial.println("Forward call through the remote");
          }
          break;
        case STOP:                               //Play/Pause
          if(usingRemote == true){stopMotor();}
          break;
        case RIGHT:                              //Volume Up
          if(usingRemote == true){
            right();
            delay(250);
            stopMotor();
          }
          break;
        case LEFT:
          if(usingRemote == true){
            left();
            delay(250);
            stopMotor();
          }
          break;
        case BACKWARD: 
          if(usingRemote == true){backward();}
          break;
        case REMOTETOGGLE:
          if(usingRemote == false){
            usingRemote = true;
            stopMotor();
          } else {usingRemote = false;}
          Serial.println(usingRemote);
          break;
      }
    }
    My_Receiver.resume(); //Restart the receiver
  }
  
  if (millis() > ldrTimer && usingRemote == false ){
    // 10 ms or 150 ms timer?
     if ( ldrInterval == 150 ){
       ldrInterval = 10;
       ldrTimer = millis() + ldrInterval ; 
       digitalWrite(LED_EDGE_DETECT_2, HIGH);
       digitalWrite(LED_EDGE_DETECT_1, HIGH);
     } else {
       ldrInterval = 150;
       ldrTimer = millis() + ldrInterval;
       long ldrLedWithEmissions = analogRead(A0);
       if (ldrLedWithEmissions < 510 ){
         digitalWrite(LED_EDGE_DETECT_2, LOW);
         digitalWrite(LED_EDGE_DETECT_1, LOW);
         stopAndBackup();
       } else {
         digitalWrite(LED_EDGE_DETECT_2, LOW);
         digitalWrite(LED_EDGE_DETECT_1, LOW);
       }
     }
  }

  if ( millis() > pingTimer  && usingRemote == false) {
      pingTimer = millis() + 250;
      digitalWrite(PING, LOW);
      delayMicroseconds(2);
      digitalWrite(PING, HIGH);
      delayMicroseconds(2);
      digitalWrite(PING, LOW);

      long duration = pulseIn(ECHO, HIGH);
      cm = microsecondsToCentimeters(duration);
  }
 
  if (digitalRead(LEFT_BUMPER) == HIGH || digitalRead(RIGHT_BUMPER) == HIGH || cm <= 10  && usingRemote == false){
    stopAndBackup();
  }
}

// -- end of main loop ---

void remoteReact(){                            // react to IR remote commands (should do same thing as switch logic... in theory)
  if (My_Receiver.GetResults(&My_Decoder)){    // if IR reciever returns results
    My_Decoder.decode();                       // decode those results
    My_Decoder.DumpResults();                  // dump result?
    if (My_Decoder.decode_type == NEC) {       // given we are getting a result from the right remote? can we take out this condition?
      if(usingRemote){                         // when using the remote
        if(My_Decoder.value == FORWARD){       // 1? button
          forward();
          Serial.println("Forward call through the remote");
        } else if (My_Decoder.value == STOP){  // 5? button
          stopMotor();
        } else if (My_Decoder.value == RIGHT){ // 6? button
          right();
          delay(250);
          stopMotor();
        } else if (My_Decoder.value == LEFT){  // 4? button
          left();
          delay(250);
          stopMotor();
        } else if (My_Decoder.value == BACKWARD){ // 8? button
          backward();
        }
      }
      if(My_Decoder.value == REMOTETOGGLE){ // whether using remote or not check if we want to
        usingRemote = !usingRemote;         // toggle using remote from previous state
        if(usingRemote){stopMotor();}       // make sure motors are stopped when taking control
        Serial.println(usingRemote);
      }
    }
  }
  My_Receiver.resume(); //Restart the receiver
}

void stopAndBackup(){
  stopMotor();
  backward();
  stopMotor();
  if((millis() % 2) == 0 ){
    left();
  } else {
    right();
  }
  stopMotor();
  delay(1000);
  forward();
}

/*
 * two of pins being writen actually only need digitalWrite,
 * if using non-pwm pin this routine will fail, because it expects to be able to analog write
 * which is completly ok, h-bridge can recieve an analog value for its digital direction pins,
 * but as far as its concerned it just got a high or a low no mater the voltage
 * in fact if the pwm was slow enough and h-bridge sampled fast enough the motors would never go anywhere either 
 * They would never be able to ramp up fast enough to go either direction
 * AKA the pulse would constantly change the direction too fast to move
 */

void stopMotor(){
  analogWrite(AIA, 0);
  analogWrite(AIB, 0);
  analogWrite(BIA, 0);
  analogWrite(BIB, 0);
}
 
void backward(){
  analogWrite(AIA, 0);
  analogWrite(AIB, speed);
  analogWrite(BIA, 0);
  analogWrite(BIB, speed);
}

void forward(){
  analogWrite(AIA, speed);
  analogWrite(AIB, 0);
  analogWrite(BIA, speed + 20);
  analogWrite(BIB, 0);
}

void right(){
  analogWrite(AIA, halfspeed);
  analogWrite(AIB, 0);
  analogWrite(BIA, 0);
  analogWrite(BIB, halfspeed);
}

void left(){
  analogWrite(AIA, 0);
  analogWrite(AIB, halfspeed);
  analogWrite(BIA, halfspeed);
  analogWrite(BIB, 0);
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

