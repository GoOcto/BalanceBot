 /**
 * This Arduino code will receive messages from the BalanceBot Android app
 * 
 * see MainActivity.kt file for an explanation of the protocol
 * 
 * Maximum length of a message is currently 5 bytes (+null), 
 * but we might add more to the protocol
 */

#define MAXLEN 15

char inputString[MAXLEN] = "";
char zerosString[MAXLEN] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0"; // "\0" at end implied

char* inPtr = inputString;

const int ledPin = LED_BUILTIN; // will depend on board

int16_t accelData[3];  // in X, Y, Z axes
int16_t gyrosData[3];  // in X, Y, Z axes


void setup() {
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
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


int16_t AsciiToInt(char* str) {
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

void interpretSensorData(int i) {
  // inputString should be exactly 5 bytes long
  accelData[i] = ((inputString[1]&0x7f)<<8) + inputString[2];
  gyrosData[i] = ((inputString[3]&0x7f)<<8) + inputString[4];

  if ( inputString[1]&0x80 ) accelData[i] = -accelData[i];
  if ( inputString[3]&0x80 ) gyrosData[i] = -gyrosData[i];
}

void interpretHumanReadableSensorData(int i) {
  // incoming sensor data: Z±AAAAA±GGGGG
  accelData[i] = AsciiToInt( inputString+1 );
  gyrosData[i] = AsciiToInt( inputString+7 );
}


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

}