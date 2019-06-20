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

#ifndef __D0GSTEP__
#define __D0GSTEP__

#include <inttypes.h>

class d0gStep {
  public:
    d0gStep(uint8_t _maxMotors);
    bool attachMotor(uint8_t _step, uint8_t _dir, uint8_t _en);
    void setOneSpeed(uint8_t _motor, unsigned int _speed);
    void moveOneMotor(uint8_t _motor, long _movement);
    void moveOneMotorTo(uint8_t _motor, long _movement);
    void stopOneMotor(uint8_t _motor);
    void moveMotor(long _target[3]);
    static void step();
    long currentPosition(uint8_t _motor);
    long targetPosition(uint8_t _motor);
    long distanceToGo(uint8_t _motor);
    void setCurrentPosition(uint8_t _motor, long _position);
    
  private:
    uint8_t motorMax;
    uint8_t motorCount;
    uint8_t d0gStepDirPin[3];
  
  protected:
    static volatile long _pos[3];
    static volatile long _targ[3];
    static uint8_t d0gStepStepPin[3];
    static volatile unsigned long dy2[3];
    volatile static unsigned int dx2; //Microseconds per call
    static volatile long D[3];
    static volatile int8_t dir[3];
    
    
};
#endif
