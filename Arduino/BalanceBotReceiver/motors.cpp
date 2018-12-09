#include "motors.h"

//Motors::Motors() : max(255) { }

Motors::Motors(int MMD, int AEN, int APH, int BEN, int BPH) {
  pinMMD = MMD;
  pinAEN = AEN;
  pinAPH = APH;
  pinBEN = BEN;
  pinBPH = BPH;
  
  pinMode(pinMMD,OUTPUT);
  pinMode(pinAEN,OUTPUT);
  pinMode(pinAPH,OUTPUT);
  pinMode(pinBEN,OUTPUT);
  pinMode(pinBPH,OUTPUT);
  //digitalWrite(MMD,LOW); // selects IN/IN mode, may need additional programming
  digitalWrite(pinMMD,HIGH); // selects PH/EN mode
};

void Motors::setLSpeed(int16_t s) {
  digitalWrite(pinAPH, (s>0) ? HIGH : LOW );
  if ( s<0 ) s = -s;
  if ( s>max ) s = max;
  analogWrite(pinAEN,s);
};

void Motors::setRSpeed(int16_t s) {
  digitalWrite(pinBPH, (s>0) ? HIGH : LOW );
  if ( s<0 ) s = -s;
  if ( s>max ) s = max;
  analogWrite(pinBEN,s);
};

void Motors::setSpeeds(int16_t l, int16_t r) {
  setLSpeed(l);
  setRSpeed(r);
};
