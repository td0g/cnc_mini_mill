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

d0gStep::d0gStep(uint8_t _maxMotors)
{    
  motorMax = _maxMotors;
  dx2 = 256;  //Double dx, which is 128 microseconds
}

bool d0gStep::attachMotor(uint8_t _step, uint8_t _dir, uint8_t _en){
  if (motorCount == motorMax) return false;
  TIMSK2 = TIMSK2 | 0b00000001;
  TCCR2B = (TCCR2B & 0b11111000) | 0b00000010;  //Trigger every 128 microseconds
  d0gStepStepPin[motorCount] = _step;
  d0gStepDirPin[motorCount] = _dir;
  pinMode(_step, OUTPUT);
  pinMode(_dir, OUTPUT);
  pinMode(_en, OUTPUT);
  digitalWrite(_en, 0);
  motorCount++;
  return true;
}

void d0gStep::setOneSpeed(uint8_t _motor, unsigned int _speed){
  dy2[_motor] = 2000000 / _speed;
  D[_motor] = dy2[_motor] - dx2 / 2;
}

void d0gStep::moveOneMotor(uint8_t _motor, long _movement){
  long _t[3];
  for (byte i = 0; i< 3; i++){
    _t[i] = _targ[i];
    if (i == _motor) _t[i] += _movement;
  }
  moveMotor(_t);
}
  
void d0gStep::moveOneMotorTo(uint8_t _motor, long _movement){
    long _t[3];
    for (byte i = 0; i< 3; i++){
      _t[i] = _targ[i];
      if (i == _motor) _t[i] = _movement;
    }
    moveMotor(_t);
  }

  void d0gStep::stopOneMotor(uint8_t _motor){
    _targ[_motor] = _pos[_motor];
    delayMicroseconds(4);  //Just in case we are in the middle of an interrupt routine - let's try twice.
    _targ[_motor] = _pos[_motor];
  }
  
  void d0gStep::moveMotor(long _target[3]){
    for (byte i = 0; i < 3; i++){
      _targ[i] = _target[i];
      if (_pos[i] < _targ[i]){
        dir[i] = 1;
        digitalWrite(d0gStepDirPin[i], 0);
      }
      else {
        dir[i] = -1;
        digitalWrite(d0gStepDirPin[i], 1);
      }
    }
  }

  long d0gStep::currentPosition(uint8_t _motor){
    return _pos[_motor];
  }
  long d0gStep::targetPosition(uint8_t _motor){
    return _targ[_motor];
  }
  long d0gStep::distanceToGo(uint8_t _motor){
    return _targ[_motor]-_pos[_motor];
  }
  void d0gStep::setCurrentPosition(uint8_t _motor, long _position){
    long delta;
    delta = _position - _pos[_motor];
    _pos[_motor] = _position;
    _targ[_motor] += delta;
  }

  void d0gStep::step(){
    for (byte i = 0; i < 3; i++){
      if (_pos[i] != _targ[i]){
        D[i] = D[i] + dx2;
        if (D[i] > 0){
          digitalWrite(d0gStepStepPin[i], 1);
          _pos[i] += dir[i];
          D[i] = D[i] - dy2[i];
        }
      }
    }
    for (byte i = 0; i < 3; i++) digitalWrite(d0gStepStepPin[i], 0);
  }

  ISR(TIMER2_OVF_vect){
    d0gStep::step();
  }

volatile long d0gStep::_targ[3];
volatile long d0gStep::_pos[3];
volatile unsigned long d0gStep::dy2[3];
volatile unsigned int d0gStep::dx2;
volatile long d0gStep::D[3];
uint8_t d0gStep::d0gStepStepPin[3];
volatile int8_t d0gStep::dir[3];
