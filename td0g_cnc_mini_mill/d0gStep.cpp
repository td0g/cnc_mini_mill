/*
  d0gStep Stepper Driver library for Arduino
  Written by Tyler Gerritsen
  vtgerritsen@gmail.com
  www.td0g.ca
 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "d0gStep.h"
#include <Arduino.h>

//#define TEST_SPEED
#define NONATOMIC_IS_UNSAFE TCNT2>0xf8  //Timer rollover at 0xFF
#define CLOCK_PRESCALER 0b00000010
  /// 001 = /1, 010 = /8, 011 = /32, 100 = /64, 101 = /128, 110 = /256, 111 = /1024

d0gStep::d0gStep(uint8_t _maxMotors, uint8_t _disableTime, uint8_t _en, uint8_t _queueSize)
{
  queueSize = _queueSize;
  pinEn = _en;
  maxMotors = _maxMotors;    
  disableTimer = 0;
  
  pinStep = (uint8_t *)malloc(_maxMotors);
  pinDir = (uint8_t *)malloc(_maxMotors);
  maxStepPerSecond = (uint16_t *)malloc(_maxMotors * 2);
  D = (int32_t *)malloc(_maxMotors * 4);
  maxPos = (int32_t *)malloc(_maxMotors * 4);
  minPos = (int32_t *)malloc(_maxMotors * 4);
  pos = (int32_t *)malloc(_maxMotors * 4);
  targ = (int32_t *)malloc(_maxMotors * 4 * _queueSize);
  dy2 = (int32_t *)malloc(_maxMotors * 4 * _queueSize);

//Following does NOT work, but saves 100 bytes
  //targ = (int32_t *)malloc(_maxMotors * 8 * _queueSize);
  //int32_t * dy2 = targ + _maxMotors * 4 * _queueSize;
  dx2 = 512;  //Double dx, which is 256 microseconds... 
}

void d0gStep::printDebug(){
    Serial.print(F("  dx2:"));
    Serial.print(dx2);
    Serial.print(F(" dy2:"));
    Serial.print(dy2[0]);
    Serial.print(F(","));
    Serial.print(dy2[1]);
    Serial.print(F(","));
    Serial.print(dy2[2]);
    Serial.print(F(" dtg:"));
    Serial.print(distanceToGo(0));
    Serial.print(F(","));
    Serial.print(distanceToGo(1));
    Serial.print(F(","));
    Serial.println(distanceToGo(2));
    #ifdef TEST_SPEED
    Serial.print(F("Max/Av us:"));
    Serial.print(maxTimeToStep);
    Serial.print(F(" / "));
    Serial.print(minTimeToStep / timeBetweenStep);
    Serial.print(" (");
    Serial.print(micros() / timeBetweenStep);
    Serial.println(")");
    #endif
}

bool d0gStep::attachMotor(uint8_t _step, uint8_t _dir){

  #if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_PRO) || defined(ARDUINO_AVR_MINI)
    //TCCR2A = 0; //128us @ 16 MHz, 256us @ 8MHz
    TCCR2A = 1;   //1 = 256us @ 16 MHz, 512us @ 8MHz
    TIMSK2 = 0b00000001;  //Enable overflow interrupt
    TCCR2B = CLOCK_PRESCALER;  //CLK / 8 - Trigger every 256 microseconds
  #elif defined(ARDUINO_AVR_LILYPAD)
    TCCR2A = 0; //128us @ 16 MHz, 256us @ 8MHz
    //TCCR2A = 1;   //1 = 256us @ 16 MHz, 512us @ 8MHz
    TIMSK2 = 0b00000001;  //Enable overflow interrupt
    TCCR2B = CLOCK_PRESCALER;  //CLK / 8 - Trigger every 256 microseconds
  #elif defined(ARDUINO_AVR_MEGA)       
      #error "d0gStep NOT COMPATIBLE WITH BOARD"
  #elif defined(ARDUINO_AVR_MEGA2560)       
      #error "d0gStep NOT COMPATIBLE WITH BOARD"
   #else
    #error "d0gStep NOT COMPATIBLE WITH BOARD"
  #endif
  
  //Sanity checks
  if (motorCount == maxMotors) return false;
  if (digitalPinToPort(_dir) == NOT_A_PIN || digitalPinToPort(_step) == NOT_A_PIN) return false;
  pinDir[motorCount] = _dir;
  pinStep[motorCount] = _step;
  maxPos[motorCount] = 0x7FFFFFFF; // 2147483647
  minPos[motorCount] = 0x80000000; // -2147483648
  maxStepPerSecond[motorCount] = 0x4FFF;
  pos[motorCount] = 0;
  //for (byte i = 0; i < queueSize; i++){
    targ[motorCount] = 0;
    dy2[motorCount] = 0;
  //}
  D[motorCount] = 0;
  motorCount++;
  pinMode(pinEn, OUTPUT);
  pinMode(_step, OUTPUT);
  pinMode(_dir, OUTPUT);
  return true;
}


int16_t d0gStep::currentSpeed(uint8_t _motor){  //Cannot decelerate from 2849... s = 2000000 / dy2 = 2849 = dy2 = 702
  if (pos[_motor] == targ[_motor + queuePos]) return 0;
  int32_t s = dyFactor;
  s /= dy2[_motor + queuePos];
  if (pos[_motor] > targ[_motor + queuePos]) s *= -1;
  return s;
}

void d0gStep::moveMotor(uint8_t _motor, int16_t _speed){
  if (!_speed) {
    stopMotor(_motor);
    return;
  }
  dy2[_motor + queuePos] = dyFactor;  //Effectively pause motor
  while (NONATOMIC_IS_UNSAFE){};
  if (_speed < 0) {
    _speed *= -1;
    targ[_motor + queuePos] = -2147483647;
  }
  else targ[_motor + queuePos] = 2147483647;
  _speed = min(_speed, maxStepPerSecond[_motor]);
  dy2[_motor + queuePos] = dyFactor / _speed;
  D[_motor] = dx2 /2 - dy2[_motor + queuePos];  //Reset bresenham algorithm
}

void d0gStep::accelerate(uint8_t _motor, int16_t _speed, uint8_t multiplier){ //_speed must be 32-bit so it can calculate from dyFactor
  int32_t _previousSpeed = currentSpeed(_motor);  //2849
  if (!_previousSpeed || abs(targ[_motor + queuePos]) != 2147483647) return;
  _previousSpeed = _speed + abs(_previousSpeed); //dyFactor / dy2[_motor + queuePos];
  //_speed = min(_speed, maxStepPerSecond[_motor]);
  if (_speed < 0){
    if (_previousSpeed < 1) stopMotor(_motor);
  }
  else if (_previousSpeed * 64 > (unsigned long)maxStepPerSecond[_motor] * multiplier) return;
  else {
    while (NONATOMIC_IS_UNSAFE){};
    _previousSpeed = dyFactor / _previousSpeed;
    if (_speed < 0) dy2[_motor + queuePos] = max(_previousSpeed, dy2[_motor + queuePos] + 1); //At higher speeds, there are cases where the acceleration / deceleration amount isn't enough to change dy2, causing an endless loop
    else dy2[_motor + queuePos] = min(_previousSpeed, dy2[_motor + queuePos] - 1);
  }
}

uint8_t d0gStep::queues(){
  return queueSize;
}

bool d0gStep::canQueue(){
  if (queueAvailable + 1 == queueSize) return false;
  return true;  
}

bool d0gStep::queueMotorsMovement(long _target[], float _time){
  if (queueAvailable + 1 == queueSize) return false;
  while (NONATOMIC_IS_UNSAFE){};
  byte lastQueuePos = (queuePos + queueAvailable*maxMotors) % (queueSize * maxMotors);
  byte thisQueuePos = (lastQueuePos + maxMotors) % (queueSize * maxMotors);
//Non-atomic is now safe, the queueAvailable variable will activate all the queued move
  float reducer = 1.0;
  float distToGo;
  //Serial.println(_time);
  for (byte i = 0; i < maxMotors; i++){
    if (_target[i] == noMovement) targ[i + thisQueuePos] = targ[i + lastQueuePos];
    else {
      targ[i + thisQueuePos] = _target[i];
      distToGo = abs(_target[i] - targ[i + lastQueuePos]);  //DistanceToGo
      if (distToGo){
        dy2[i + thisQueuePos] = dyFactor * _time / distToGo;
        if (distToGo > reducer * maxStepPerSecond[i] * _time) reducer = distToGo / maxStepPerSecond[i] / _time;
      }
    }
    //Serial.print(distToGo);
    //Serial.print(" ");
    //Serial.println(dy2[i+thisQueuePos]);
  }
  for (byte i =  thisQueuePos; i < maxMotors + thisQueuePos; i++)dy2[i] = dy2[i] * reducer;
  queueAvailable++;
  return true;
}

bool d0gStep::interruptSoon(){
  if (TCNT2 > 0xd0) return true;
  return false;
}

int32_t d0gStep::lastQueuedTarg(uint8_t _motor){
  byte lastQueuePos = (queuePos + queueAvailable*maxMotors) % (queueSize * maxMotors);
  return targ[_motor + lastQueuePos];
}

  void d0gStep::stopMotor(uint8_t _motor){
    while (NONATOMIC_IS_UNSAFE){};
    for (byte i = 0; i < queueSize; i++)targ[_motor + i * maxMotors] = pos[_motor];
  }

  long d0gStep::currentPosition(uint8_t _motor){
    while (NONATOMIC_IS_UNSAFE){};
    return pos[_motor];
  }

  long d0gStep::targetPosition(uint8_t _motor){
    return targ[_motor + queuePos];
  }

  long d0gStep::distanceToGo(uint8_t _motor){
    while (NONATOMIC_IS_UNSAFE){};
    return targ[_motor + queuePos] - pos[_motor];
  }

  uint8_t d0gStep::moving(uint8_t _motor){
    if (_motor >= motorCount){
      if (queueAvailable) return 1;
      for (byte i = 0; i < motorCount; i++){
        if (pos[i] != targ[i + queuePos]) return 1;
      }
    }
    else if (currentSpeed(_motor) != 0) return 1;
    return 0;
  }

  void d0gStep::setMotorPosition(uint8_t _motor, long _position){
    long delta;
    while (NONATOMIC_IS_UNSAFE){};
    delta = _position - pos[_motor];
    pos[_motor] = _position;
    targ[_motor + queuePos] += delta;
  }

  uint16_t d0gStep::maxSpeed(uint8_t _motor, uint16_t _maxSpeed){
    if (_maxSpeed){
      _maxSpeed = min(_maxSpeed, maxSpeedLimit);
      maxStepPerSecond[_motor] = _maxSpeed;
    }
    return maxStepPerSecond[_motor];
  }

  void d0gStep::setMotorMaxPosition(uint8_t _motor, long _position){
    while (NONATOMIC_IS_UNSAFE){};
    maxPos[_motor] = _position;
  }

  void d0gStep::setMotorMinPosition(uint8_t _motor, long _position){
    while (NONATOMIC_IS_UNSAFE){};
    minPos[_motor] = _position;
  }

  void d0gStep::setMotorMaxPositionHere(uint8_t _motor){
    while (NONATOMIC_IS_UNSAFE){};
    maxPos[_motor] = pos[_motor];
  }

  void d0gStep::setMotorMinPositionHere(uint8_t _motor){
    while (NONATOMIC_IS_UNSAFE){};
    minPos[_motor] = pos[_motor];
  }

  void d0gStep::getMotorMaxPosition(uint8_t _motor){
    return maxPos[_motor];
  }
  
  void d0gStep::getMotorMinPosition(uint8_t _motor){
    return maxPos[_motor];
  }

  bool d0gStep::nudge(uint8_t _motor, int _dist, float _speed){
    if (TCCR2B) return false;
    _speed = 500000 / _speed;
    if (_dist < 0){
      writeFast(pinDir[_motor], 1);
      _dist = _dist * -1;
    }
    else writeFast(pinDir[_motor], 0);
    while (_dist){
      _dist--;
      delayMicroseconds(_speed);
      writeFast(pinStep[_motor], 1);
      delayMicroseconds(_speed);
      writeFast(pinStep[_motor], 0);
    }
    if (targ[_motor + queuePos] > pos[_motor]) writeFast(pinDir[_motor], 0);
    else writeFast(pinDir[_motor], 1);
    return true;
  }

  void d0gStep::enable(uint8_t _e){
    if (_e) TCCR2B = CLOCK_PRESCALER;
    else TCCR2B = 0;
  }

  void d0gStep::disable(){
    enable(0);
  }


  void d0gStep::step(){
        #ifdef TEST_SPEED
          unsigned long _t = micros();
        #endif
    static byte _timer;
    _timer++;
    if (!_timer){
      disableTimer++;
      if ((disableTimer >> disableTimerFactor) && disableTimer)writeFast(pinEn, 1); 
    }
    bool stepsRemaining = false;
    //Set DIR pin states and check if any motors have not reached their destination
    for (byte i = 0; i < motorCount; i++){
      if (targ[i + queuePos] != pos[i]) {
        if (pos[i] < targ[i + queuePos]) writeFast(pinDir[i], 0); //Set DIR pin
        else writeFast(pinDir[i], 1);                             //Set DIR pin
        D[i] = D[i] + dx2;      //Part of the bresenham algorithm - must only be performed once per function call
        stepsRemaining = true;  //Flag that motor(s) have not reached their destination
        disableTimer = 0;       //Resets timer
      }
    }
    //28 micro
    if (stepsRemaining) writeFast(pinEn, 0);  //Some motors have not reached their destination, make sure motors are enabled
    else if (queueAvailable) {  //All motors have reached their destination - is a movement queued?
      queuePos = (queuePos + maxMotors) % (queueSize * maxMotors);
      queueAvailable--;      
      for (byte i = 0; i < motorCount; i++) D[i] = dx2 - dy2[i + queuePos] / 2;
      TCNT2 = 254;  //Get out, then let interrupt occur almost immediately with new destination.
      return;
    }
    //0 micro
    while (stepsRemaining) {  //Bresenham algorithm
      stepsRemaining = false;
      for (byte i = 0; i < motorCount; i++){
        if (D[i] > 0 && targ[i + queuePos] != pos[i]){
          if (pos[i] < maxPos[i] && pos[i] > minPos[i]) writeFast(pinStep[i], 1);
          if (targ[i + queuePos] > pos[i]) pos[i]++;
          else pos[i]--;
          D[i] = D[i] - dy2[i + queuePos];
          if (D[i] > 0) stepsRemaining = true;
        }
      }
      for (byte i = 0; i < motorCount; i++) writeFast(pinStep[i],0);
    }
    //50 micro
        #ifdef TEST_SPEED
          _t = micros() - _t;
          maxTimeToStep = max(_t, maxTimeToStep);
          minTimeToStep += _t;
          timeBetweenStep++;
        #endif
  }

  ISR(TIMER2_OVF_vect){
    d0gStep::step();
  }

  static void d0gStep::writeFast(uint8_t _pin, uint8_t _value){ 
    uint8_t bit = digitalPinToBitMask(_pin);
    uint8_t port = digitalPinToPort(_pin);
    volatile uint8_t *out;
    out = portOutputRegister(port);
    if (!_value) *out &= ~bit;
    else *out |= bit;
  }

  uint8_t d0gStep::maxMotors;
  volatile long *d0gStep::targ;
  volatile long *d0gStep::pos;
  volatile unsigned long *d0gStep::dy2;
  volatile uint16_t *d0gStep::maxStepPerSecond;
  volatile unsigned int d0gStep::dx2;
  unsigned int d0gStep::dx2Max;
  unsigned int d0gStep::dx2Delta;
  volatile long *d0gStep::D;
  uint8_t d0gStep::motorCount;
  uint8_t *d0gStep::pinDir;
  uint8_t *d0gStep::pinStep;
  uint8_t d0gStep::pinEn;
  volatile uint8_t d0gStep::queueAvailable = 0;
  long *d0gStep::maxPos;
  long *d0gStep::minPos;
  volatile uint16_t d0gStep::disableTimer;
  volatile uint8_t d0gStep::queuePos;
  uint8_t d0gStep::queueSize;
  uint8_t d0gStep::disableTimerFactor;
  #ifdef TEST_SPEED
    volatile uint16_t d0gStep::maxTimeToStep;
    volatile uint16_t d0gStep::minTimeToStep = 65530;
    volatile uint32_t d0gStep::timeBetweenStep;
  #endif
