 /**
 * This Arduino code will receive messages from the BalanceBot Android app
 * 
 * Built and Programmed by the LogicalOctopus
 * On Github at https://github.com/GoOcto/BalanceBot
 * 
 * 
 * 
 * Uses a very simple protocol:
 *
 *  efficient sensor info: (5 chars (bytes) each)
 *  "U" + Accel.asInt16 + Gyros.asInt16  on the X axis
 *  "V" + Accel.asInt16 + Gyros.asInt16  on the Y axis, this is the most important one for balancing
 *  "W" + Accel.asInt16 + Gyros.asInt16  on the Z axis
 *
 *  human readable sensor info: (easier for testing, see below for format) (13 chars (bytes) each)
 *  "X" + human readable Accel data + human readable Gyros data on the X axis
 *  "Y" + human readable Accel data + human readable Gyros data on the Y axis
 *  "Z" + human readable Accel data + human readable Gyros data on the Z axis
 *
 *  - Accel data is multiplied by 1000 so 1 G is roughly 9810 units
 *  - Gyros data is multiplied by 1000 so 1°/sec is 1000 units
 *  
 *  "M" + humanreadable motor data in Left, Right order (ie. M+200-050 sets the Left motor to forward at a PWM of 200/255 
 *            and the right motor to backward at a PWM of 50/255)
 *
 *  or the following Strings:
 *  "HIGH"   put the Arduino's internal LED into HIGH mode  (may be either On or OFF depending on Arduino model)
 *  "LOW"    put the Arduino's internal LED into LOW mode
 *  "READ"   requests a test data packet back from Arduino
 *
 *  All messages must end with "\n" to signal the end of a single transmission
 *  The Arduino interprets everything between "\n" and the next "\n" as a complete message
 *
 *  The human readable data for sensors must be in the form:
 *  ±NNNNNN  where N must be digits 0..9 and the values range between -32768..+32767 (an int16)
 *  
 *  and for motors it is similar but shorter:
 *  ±NNN  where N must be digits 0..9 and the values range between -255..+255 (the min and max value for the motor PWM)
 *
 *  Notice that the first character of each signal is unique.
 */

#include "motors.h"


// These parameters may need to be tweaked for calibration
#define     GEAR_RATIO      120
#define  ANGLE_RATE_RATIO   180
#define    ANGLE_RESPONSE    11
#define DISTANCE_RESPONSE    65  /* 73 */
#define    SPEED_RESPONSE  3300

// send feedback on the Serial channel
#define SERIAL_DEBUG  1


// Pin Mappings for Motors
#define MMD   9
#define BEN   6 
#define BPH   5 
#define AEN  11
#define APH  10

int32_t driveL;
int32_t driveR;

// Pin Mappings for Encoders (xEA pin must be able to respond to a CHANGE Interrupt)
#define REA   2
#define REB   7
#define LEA   3
#define LEB   8

volatile int32_t countL = 0;
volatile int32_t countR = 0;




// communications/protocol 
#define MAXLEN 15

char inputString[MAXLEN] = "";
char zerosString[MAXLEN] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0"; // "\0" at end implied

char* inPtr = inputString;

const int ledPin = LED_BUILTIN; // will depend on board

int16_t accelData[3];  // in X, Y, Z axes
int16_t gyrosData[3];  // in X, Y, Z axes





Motors motors(MMD,AEN,APH,BEN,BPH);

int motorSpeed[2];


void doCountL() {
  int a = digitalRead(LEA);
  int b = digitalRead(LEB);
  countL += (a==b) ? -1 : +1;
}


void doCountR() {
  int a = digitalRead(REA);
  int b = digitalRead(REB);
  countR += (a==b) ? +1 : -1;
}












// -----------------------------------------------------------------------------------
void setup() {
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  // set up encoders to count wheel movement
  pinMode(LEA,INPUT_PULLUP);
  pinMode(REA,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEA),doCountL,CHANGE);
  attachInterrupt(digitalPinToInterrupt(REA),doCountR,CHANGE);

  motors.setSpeeds(0,0);
  
  delay(200);

  for (int i=0;i<10;i++) {
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
  }

  Serial.print("Ready to go on pin ");
  Serial.println(ledPin);

}


// -----------------------------------------------------------------------------------
int16_t Ascii6ToInt(char* str) {
  // always 6 chars long including the sign
  int16_t n=0;
  // interpret the digits
  n += (str[1]-'0')*10000;
  n += (str[2]-'0')*1000;
  n += (str[3]-'0')*100;
  n += (str[4]-'0')*10;
  n += (str[5]-'0');
  // now the sign
  if ( str[0]=='-' ) n = -n;
  return n;
}

// -----------------------------------------------------------------------------------
int16_t Ascii4ToInt(char* str) {
  // always 4 chars long including the sign
  int16_t n=0;
  // interpret the digits
  n += (str[1]-'0')*100;
  n += (str[2]-'0')*10;
  n += (str[3]-'0');
  // now the sign
  if ( str[0]=='-' ) n = -n;
  return n;
}

// -----------------------------------------------------------------------------------
void interpretSensorData(int i) {
  // inputString should be exactly 5 bytes long
  accelData[i] = ((inputString[1]&0x7f)<<8) + inputString[2];
  gyrosData[i] = ((inputString[3]&0x7f)<<8) + inputString[4];

  if ( inputString[1]&0x80 ) accelData[i] = -accelData[i];
  if ( inputString[3]&0x80 ) gyrosData[i] = -gyrosData[i];
}

// -----------------------------------------------------------------------------------
void interpretHumanReadableSensorData(int i) {
  // incoming sensor data: Z±AAAAA±GGGGG
  accelData[i] = Ascii6ToInt( inputString+1 );
  gyrosData[i] = Ascii6ToInt( inputString+7 );
}

// -----------------------------------------------------------------------------------
void interpretHumanReadableMotorData() {
  // incoming sensor data: M±AAA±GGG
  motorSpeed[0] = Ascii4ToInt( inputString+1 );
  motorSpeed[1] = Ascii4ToInt( inputString+5 );
}


// -----------------------------------------------------------------------------------
void handleInputString() {

    //Serial.print("Proc: (");
    //Serial.print(strlen(inputString));
    //Serial.print(")");
    //Serial.println(inputString);

    switch ( inputString[0] ) {

      case 'U':
        interpretSensorData(0);
        break;
        
      case 'V':
        interpretSensorData(1);
        break;
        
      case 'W':
        interpretSensorData(2);
        break;

      case 'M':
        interpretHumanReadableMotorData();
        motors.setLSpeed(motorSpeed[0]);
        motors.setRSpeed(motorSpeed[1]);
        break;        
        
      case 'X':
        interpretHumanReadableSensorData(0);
        break;
        
      case 'Y':
        interpretHumanReadableSensorData(1);
        break;
        
      case 'Z':
        interpretHumanReadableSensorData(2);
        break;
        
      default:
        // these don't need to be fast
        if ( strcmp(inputString,"LOW")==0 ) {
            digitalWrite(ledPin, LOW);
        }
        else if ( strcmp(inputString,"HIGH")==0 ) {
            digitalWrite(ledPin, HIGH);
        }
        else if ( strcmp(inputString,"READ")==0 ) {
            Serial.println("Message from Arduino");
        }
        break;
        
    }
    
}


// -----------------------------------------------------------------------------------
void balanceDrive(int16_t motorL, int16_t motorR) {
  driveL = motorL;
  driveR = motorR;
}


// -----------------------------------------------------------------------------------
void loop() {


  // we need to handle incoming Serial data as quickly as possible
  while (Serial.available()) { 
   
    // read incoming data
    char inChar = (char)Serial.read(); 

    if ( inChar=='\n' ) {
      handleInputString();
      
      // ...and start again
      inPtr = inputString;
      strncpy(inputString,zerosString,MAXLEN);
    }
    else {
      // add it to the inputString:
      *inPtr++ = inChar;

      if ( (inPtr-inputString) >= (MAXLEN-1) ) {
        // this is too long to be legit, abort
        inPtr = inputString;
        strncpy(inputString,zerosString,MAXLEN);
      }
    }


  }

  //Serial.print("counts: ");
  //Serial.print(countL);
  //Serial.print(", ");
  //Serial.println(countR);
  

}
