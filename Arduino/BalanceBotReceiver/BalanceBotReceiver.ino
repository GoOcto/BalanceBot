

String inputString = "";


const int ledPin = LED_BUILTIN;


void setup() {
  inputString.reserve(200);
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  delay(2000);

  for (int i=0;i<10;i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
  Serial.print("Ready to go on pin ");
  Serial.println(LED_BUILTIN);
  
}


void loop() {
  delay(10);
  
  while (Serial.available()) { 
   
    // Read incoming Control From App Inventor
    char inChar = (char)Serial.read(); 
    //Serial.print(inChar);

    if ( inChar=='\n' ) {

      Serial.print("Recvd: ");
      Serial.println(inputString);
      
      // process the incoming string
      if ( inputString=="READ" ) {
        Serial.println("Message from Arduino");
      }
      else if ( inputString=="LOW" ) {
        digitalWrite(ledPin, LOW);
      }
      else if ( inputString=="HIGH" ) {
        digitalWrite(ledPin, HIGH);
      }
      inputString = "";
    }
    else {
      // add it to the inputString:
      inputString += inChar;
    }
    
  }

/*
  char report[100];
  snprintf_P(report,sizeof(report),
      PSTR("%6ld  %6ld"),
      (long)countL, (long)countR );
  Serial.println(report);
*/    
}