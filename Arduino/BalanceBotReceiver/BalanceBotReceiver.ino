/**
 * This Arduino code will receive messages from the BalanceBot Android app
 * 
 * for now, it is only in test mode, functionality is limited
 */


#define MAXLEN 8

char inputString[MAXLEN] = "";  // should only need 4 so far, but we might add more to the protocol
int expectDataType = 0;         // 0: expect ASCI, 1: expect AccelData, 2: expect GyrosData

char* inPtr = inputString;

const int ledPin = LED_BUILTIN; // will depend on board
int32_t accelData;
int32_t gyrosData;



void setup() {
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  delay(2000);

  for (int i=0;i<10;i++) {
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
  }
  Serial.print("Ready to go on pin ");
  Serial.println(LED_BUILTIN);
  
}



int32_t inputStringAsIntBytes() {

  int32_t v = 0;
  v  = inputString[0] << 24;
  v += inputString[1] << 16;
  v += inputString[2] <<  8;
  v += inputString[3];
  
  return v;
}



void handleASCII() {

    // this might be slow, but it's good for now
    switch ( inputString[0] ) {

      case 'A':
        expectDataType = 1;
        break;

      case 'G':
        expectDataType = 2;
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
      
    }
    
}


void handleInputString() {
  
    switch ( expectDataType ) {

      case 1:
        accelData = inputStringAsIntBytes();
        expectDataType = 0;
        break;
        
      case 2:
        gyrosData = inputStringAsIntBytes();
        expectDataType = 0;
        break;
        
      default: // should be 0! but we catch everything 
        handleASCII();
        break;

    }

}




void loop() {

  // we need to handle incoming Serial data as quickly as possible
  while (Serial.available()) { 
   
    // read incoming data
    char inChar = (char)Serial.read(); 

    if ( inChar=='\n' ) {
      handleInputString();
      // ...and start again
      inPtr = inputString;
    }
    else {
      // add it to the inputString:
      *inPtr = inChar;
      inPtr++;

      if ( (inPtr-inputString) >= (MAXLEN-1) ) {
        // this is too long to be legit, abort
        inPtr = inputString;
      }
    }
    
  }

}
