/**
 * This Arduino code will receive messages from the BalanceBot Android app
 * 
 * see MainActivity.kt file for an explanation of the protocol
 * 
 * Maximum length of a message is currently 5 bytes (+null), 
 * but we might add more to the protocol
 */

#define MAXLEN 8

char inputString[MAXLEN] = "";  //

char* inPtr = inputString;

const int ledPin = LED_BUILTIN; // will depend on board
int16_t accelData;
int16_t gyrosData;


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


void interpretSensorData() {
  // inputString[0] is 'Z'
  accelData = (inputString[1]<<8) + inputString[2];
  gyrosData = (inputString[3]<<8) + inputString[4];
}


void handleInputString() {

    // this might be slow, but it's good for now
    switch ( inputString[0] ) {

      case 'Z':
        interpretSensorData()
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
