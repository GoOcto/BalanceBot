#ifndef MOTORS_H_
#define MOTORS_H_

#include "Arduino.h"

class Motors
{
  private:
    int pinMMD;
    int pinAEN;
    int pinAPH;
    int pinBEN;
    int pinBPH;
    int max = 255;
  public:
    Motors(int,int,int,int,int);
    void setLSpeed(int16_t);
    void setRSpeed(int16_t);
    void setSpeeds(int16_t,int16_t);
};




#endif /* MOTORS_H_ */
