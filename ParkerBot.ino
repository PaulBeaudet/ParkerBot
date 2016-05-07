#include <IRLib.h>
#include <IRLibMatch.h>
#include <IRLibRData.h>
#include <IRLibTimer.h>

const int IRPin = 2; 
const long FORWARD = 0xFF18E7 ; 
const long STOP = 0xFF38C7;
const long BACKWARD = 0xFF4AB5 ; 
const long RIGHT = 0xFF5AA5 ; 
const long LEFT = 0xFF10EF ; 
const long REMOTETOGGLE = 0xFFE21D;

const int AIA = 9;  // (pwm) pin 9 connected to pin A-IA
const int AIB = 5;  // (pwm) pin 5 connected to pin A-IB
const int BIA = 10; // (pwm) pin 10 connected to pin B-IA 
const int BIB = 6;  // (pwm) pin 6 connected to pin B-IB

const int LeftBumperPin = 4;
const int RightBumperPin = 3 ; 
const int pingPin = 11;
const int echoPin = 13;
const int ledPinEdgeDetector1 = 7 ;
const int ledPinEdgeDetector2 = 8 ; 

byte speed = 200;  // change this (0-255) to control the speed of the motors
byte halfspeed = 200 ;

//Create a receiver object to listen on pin 11
IRrecv My_Receiver(IRPin);

//Create a decoder object
IRdecode My_Decoder;
 

void setup() {
  
  // h bridge pins
  pinMode(AIA, OUTPUT); // set pins to output
  pinMode(AIB, OUTPUT);
  pinMode(BIA, OUTPUT);
  pinMode(BIB, OUTPUT);

// bumpers 
  pinMode(LeftBumperPin, INPUT_PULLUP) ; 
  pinMode(RightBumperPin, INPUT_PULLUP) ; 

// ultrasound pins
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT) ; 

  // edge detector setup 
  pinMode(ledPinEdgeDetector1, OUTPUT) ; 
  pinMode(ledPinEdgeDetector2, OUTPUT) ; 
 
  //forward();
 
   Serial.begin(115200) ; 
  My_Receiver.enableIRIn(); // Start the receiver
 
}

long pingTimer = 0 ; 
long ldrTimer = 0 ; 
long cm = 25 ;
long ldrNoLedEmissions = 0 ; 
int ldrInterval = 150 ; 

boolean usingRemote = false ;



void loop() {

  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();
    My_Decoder.DumpResults();
     
    if (My_Decoder.decode_type== NEC) {
           
      switch(My_Decoder.value) {
        case FORWARD:  //Volume Down
          if(usingRemote == true) 
          {
          forward() ;
          Serial.println("Forward call through the remote") ; 
          }
          break;
        case STOP:  //Play/Pause
           if(usingRemote == true) 
          {
          stopMotor() ;
          } 
          break;
        case RIGHT:  //Volume Up
          if(usingRemote == true) 
          {
            right() ; 
            delay(250) ; 
            stopMotor() ; 
          } 
          break;
        case LEFT:  
          if(usingRemote == true) 
          {
            left() ;
            delay(250) ; 
            stopMotor() ; 
          } 
          break;
        case BACKWARD: 
           if(usingRemote == true) 
          {
            backward() ; 
 
          } 
          break;
        case REMOTETOGGLE:
          if(usingRemote == false) 
          {
            usingRemote = true ;
            stopMotor() ; 
          } 
          else 
            usingRemote = false ;
          Serial.println(usingRemote) ; 
          break ;  
      }
      
    }
    My_Receiver.resume(); //Restart the receiver
  }
  
  if (millis() > ldrTimer && usingRemote == false )
  {
    // 10 ms or 150 ms timer?
     if ( ldrInterval == 150 ) 
     {
       ldrInterval = 10 ;
       ldrTimer = millis() + ldrInterval ; 
       digitalWrite(ledPinEdgeDetector2, HIGH) ; 
       digitalWrite(ledPinEdgeDetector1, HIGH) ; 
     }
     else 
     {
       ldrInterval = 150 ; 
       ldrTimer = millis() + ldrInterval ; 
       long ldrLedWithEmissions = analogRead(A0);
       
       if (ldrLedWithEmissions < 510 )
       { 
         digitalWrite(ledPinEdgeDetector2, LOW) ; 
         digitalWrite(ledPinEdgeDetector1, LOW) ;
  
         stopAndBackup() ; 
       }
       else
       {
         digitalWrite(ledPinEdgeDetector2, LOW) ; 
         digitalWrite(ledPinEdgeDetector1, LOW) ;
       }
     }
  }

  if ( millis() > pingTimer  && usingRemote == false) 
   {
      pingTimer = millis() + 250 ; 
      
      digitalWrite(pingPin, LOW);
      delayMicroseconds(2);
      digitalWrite(pingPin, HIGH);
      delayMicroseconds(2);
      digitalWrite(pingPin, LOW);

      long duration = pulseIn(echoPin, HIGH);
      cm = microsecondsToCentimeters(duration);

//      Serial.print("distance in cm = " ) ; 
//      Serial.println(cm) ; 
//      Serial.print("left bumper " ) ; 
//      Serial.println(digitalRead(LeftBumperPin)) ; 
//      Serial.print("right bumper " ) ; 
//      Serial.println(digitalRead(RightBumperPin)) ; 
  }
 
  if (digitalRead(LeftBumperPin) == HIGH || digitalRead(RightBumperPin) == HIGH || cm <= 10  && usingRemote == false)
  {
    stopAndBackup() ; 
  }
 }
void stopAndBackup() 
{
  stopMotor(1000);
    
    backward(750) ; 
     

    stopMotor(1000) ; 
    

    if ((millis() % 2) == 0 ) 
      left(250) ;
    else 
      right(250) ; 

    stopMotor(1000) ; 
    delay(1000) ; 
    
    forward() ;
  
}

void stopMotor()
{
  analogWrite(AIA, 0);
  analogWrite(AIB, 0);
  analogWrite(BIA, 0);
  analogWrite(BIB, 0);
}
 
void backward()
{
  analogWrite(AIA, 0);
  analogWrite(AIB, speed);
  analogWrite(BIA, 0);
  analogWrite(BIB, speed);
}

void forward()
{
  analogWrite(AIA, speed);
  analogWrite(AIB, 0);
  analogWrite(BIA, speed + 20);
  analogWrite(BIB, 0);
}
void right()
{
  analogWrite(AIA, halfspeed);
  analogWrite(AIB, 0);
  analogWrite(BIA, 0);
  analogWrite(BIB, halfspeed);

}
void left()
{
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
