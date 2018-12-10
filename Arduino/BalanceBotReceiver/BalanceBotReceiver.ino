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
 *  "A±NNNNN" to send Angle data where NNNNN is angle in degrees*100 (eg. 9876 is 98.76°)
 *  "G±NNNNN" to send Gyros data where NNNNN is the gyros value*1000 (eg. 12345 is 12.345°/sec)
 *  
 *  "M±NNN±NNN" to send Motors data, to be interpretted as a desire to move in a particular way
 *              this is not direct motor control, it tells the balancing algorithm how we'd like to move
 *              the values are expected PWM signals NNN/255
 *
 *  or the following Strings:
 *  "HIGH"   put the Arduino's internal LED into HIGH mode  (may be either On or OFF depending on Arduino model)
 *  "LOW"    put the Arduino's internal LED into LOW mode
 *  "READ"   requests a test data packet back from Arduino
 *
 *  All messages must end with "\n" to signal the end of a single transmission
 *  The Arduino interprets everything between "\n" and the next "\n" as a complete message
 *
 *  Notice that the first character of each signal is unique.
 */

#include "motors.h"


// These parameters may need to be tweaked for calibration
#define        GEAR_RATIO   120
#define ANGLE_CALIBRATION -1500  /* where is up gravity compared to upright phone? */
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

Motors motors(MMD,AEN,APH,BEN,BPH);


// Pin Mappings for Encoders (xEA pins must be able to respond to a CHANGE Interrupt)
#define REA   2
#define REB   7
#define LEA   3
#define LEB   8

volatile int32_t countL = 0;
volatile int32_t countR = 0;

void doCountL() { // called from an pin CHANGE interrupt
  int a = digitalRead(LEA);
  int b = digitalRead(LEB);
  countL += (a==b) ? -1 : +1;
}

void doCountR() { // called from an pin CHANGE interrupt
  int a = digitalRead(REA);
  int b = digitalRead(REB);
  countR += (a==b) ? +1 : -1;
}




// communications/protocol 
#define MAXLEN 15

char inputString[MAXLEN] = "";
char zerosString[MAXLEN] = { 0 };

char* inPtr = inputString;

int32_t gyrosData;  // on the one important axis
int32_t angleData;



// trackers for balancing
// int32_t rad2deg000 = 180000/3.14159265;
int32_t angleAccel;
int32_t angleGyro;
int32_t risingAngleOffset;
int32_t fallingAngleOffset;
bool upright;

int32_t lftS;
int32_t lftD;
int32_t rgtS;
int32_t rgtD;
int motorSpeed;
















// -----------------------------------------------------------------------------------
void setup() {
  
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // set up encoders to count wheel movement
  pinMode(LEA,INPUT_PULLUP);
  pinMode(REA,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEA),doCountL,CHANGE);
  attachInterrupt(digitalPinToInterrupt(REA),doCountR,CHANGE);

  motors.setSpeeds(0,0);
  
  delay(200);

  for (int i=0;i<10;i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }

}

// -----------------------------------------------------------------------------------
int32_t AsciiToInt(int ndigits, char* str) {
  // str need to have length of ndigits +1 for the sign
  int32_t n=0;
  int m = 1;

  // interpret the digits
  while ( ndigits>0 ) {
    n += (str[ndigits]-'0') * m;
    m *= 10;
    ndigits--;
  }
  
  // interpret the sign
  if ( str[0]=='-' ) n = -n;
  return n;
}

// -----------------------------------------------------------------------------------
void handleInputString() {

    //Serial.print("Proc: (");
    //Serial.print(strlen(inputString));
    //Serial.print(")");
    //Serial.println(inputString);

    switch ( inputString[0] ) {

      case 'A':
        // incoming angle data: A±NNNNN
        // this is calced in the Android with atan2(accelZ,accelY)*180/PI
        angleData = AsciiToInt( 5, inputString+1 );
        break;
        
      case 'G':
        // incoming gyros data: G±NNNNN
        gyrosData = AsciiToInt( 5, inputString+1 );
        doBalanceLoop();
        break;

      case 'M':
        // incoming motors data: M±NNN±NNN
        int16_t mL = AsciiToInt( 4, inputString+1 );
        int16_t mR = AsciiToInt( 4, inputString+5 );
        balanceDrive(mL,mR);
        break;        
        
      default:
        // these don't need to be fast
        if ( strcmp(inputString,"LOW")==0 ) {
          digitalWrite(LED_BUILTIN, LOW);
        }
        else if ( strcmp(inputString,"HIGH")==0 ) {
          digitalWrite(LED_BUILTIN, HIGH);
        }
        else if ( strcmp(inputString,"READ")==0 ) {
          Serial.println("Message acknowledge");
        }
        break;
        
    }
    
}

// -----------------------------------------------------------------------------------
void doBalance() {
  
  motorSpeed += (
    + ANGLE_RESPONSE * risingAngleOffset
    + DISTANCE_RESPONSE * (lftD+rgtD)
    + SPEED_RESPONSE * (lftS+rgtS)
    ) / 100 / GEAR_RATIO;

  if (motorSpeed >  255)  motorSpeed =  255;
  if (motorSpeed < -255)  motorSpeed = -255;

  motors.setSpeeds(
    motorSpeed + (rgtD-lftD)/2,
    motorSpeed + (lftD-rgtD)/2 );

}

// -----------------------------------------------------------------------------------
void doResting() {
  // Reset things so it doesn't go crazy.
  motorSpeed = 0;
  lftD = 0; lftS = 0;
  rgtD = 0; rgtS = 0;
  motors.setSpeeds(0, 0);

  if ( gyrosData>-60 && gyrosData<60 )
  {
    // get our actual angle and reset the accumulated gyro
    angleGyro = angleAccel;
  } 
  
}

// -----------------------------------------------------------------------------------
void balanceDrive(int16_t motorL, int16_t motorR) {
  driveL = motorL;
  driveR = motorR;
}

// -----------------------------------------------------------------------------------
void readSensors() {

  angleAccel = angleData + ANGLE_CALIBRATION; // straight across simple conversion
  upright = ( angleAccel>-80000 && angleAccel<80000 );

  // an estimated angle based on accumulated gyrosData
  angleGyro += gyrosData;
  angleGyro = angleGyro * 999 / 1000;

  static int16_t prevCntL;
  lftS = (countL - prevCntL);
  lftD += countL - prevCntL;
  prevCntL = countL;

  static int16_t prevCntR;
  rgtS = (countR - prevCntR);
  rgtD += countR - prevCntR;
  prevCntR = countR;

  // update speed and distance by applying the requested speeds to the counters
  lftD -= driveL;  lftS -= driveL;
  rgtD -= driveR;  rgtS -= driveR;

  risingAngleOffset  = gyrosData * ANGLE_RATE_RATIO + angleGyro;
  fallingAngleOffset = gyrosData * ANGLE_RATE_RATIO - angleGyro;

  static char buf[200];
  snprintf_P(buf,sizeof(buf), PSTR("G, FAO: %06ld  %06ld  %06ld"),
    (long)gyrosData, (long)angleGyro, (long)fallingAngleOffset);
  Serial.println(buf);
  
}

// -----------------------------------------------------------------------------------
void doBalanceLoop(){
  
  // assumed to be ~once per 20ms
  readSensors();

  if ( upright )  {
    //doBalance();
  }
  else {
    doResting();
  }
  
}

// -----------------------------------------------------------------------------------
void loop() {


  // all we do is handle incoming Serial data as quickly as possible
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

}
