/*
  d0gStep Stepper Driver library for Arduino
  Written by Tyler Gerritsen
  vtgerritsen@gmail.com
  www.td0g.ca


DESCRIPTION:
  d0gStep was written to be a general-purpose, interrupt-based stepper driver library
  The Bresenham Algorithm was implemented to reduce overhead and improve performance
  The algorithm is called by a timer overflow interrupt, which means nothing needs to be polled in the main code
D0gStep Assumptions:

Hardware:
  Using ATMEGA328P, A4988 steppers (or similar)
  Reset pin is NOT used for output
  Pin input/output state, PWM state is never altered externally
  Timer2 is available for use
  A single GPIO controls all EN pins
  Direction polarity cannot be reversed – this must be done in hardware (flipping the connector)
  Kinematics:
    The time to disable is set in stops (15s, 30s, 60s, 120s, 240s, etc…, never)
    Acceleration rate is defined by Time To Desired Speed
    Acceleration does not start from ZERO – the starting speed (jerk) is a percentage of the desired speed
    Motor position will never exceed the limits of a 32-bit integer (~+/-2.1 billion)
    Two types of motor movements may be required:
      Unified movement to target (all motors begin moving and arrive at destination at the same time)
        An example is a CNC Gcode movement, where the tool must move in a straight line and at a given speed
        Unified movements may occasionally receive a Stop signal.
        Unified movements are queued, and the next target will be loaded upon reaching the current target by all motors
        Speed is controlled by arrival time
        Acceleration is applied to all motors uniformly, maintaining the relationship between motor positions
      Independent movement without target (each motor will begin moving when told and will continue until told to stop)
        An example is moving a CNC axis using a remote control switch
        Independent movements require a stop signal
        Independent movements are never queued
        Motors can have separate start and stop times
        Speed is controlled by step rate
        Acceleration?????
    Min and Max positions are handled in the library
    If a motor reaches the min/max position, it will stop moving but the reported position will continue to increment.
    If it is reached during a unified movement, the other motors will continue moving at the same rate.
    The motor can resume motion once the reported position returns to within the min/max positions
  Issues:
    Calling moving() or distanceToGo() immediately after queueMotorsMovment() may not result in accurate



CHANGELOG:
  2019-07-16  0.1   Functional
  2020-03-02  0.2   Dynamic motor count, queueing
  2020-03-06  0.3   Dynamic queueing buffer
  2020-07-15  0.4   Fixed high-speed accel/decel bug


LICENSE:
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

  #define NEVER_DISABLE 0
  #define DISABLE_4_SECONDS 5
  #define DISABLE_8_SECONDS 6
  #define DISABLE_15_SECONDS 7
  #define DISABLE_30_SECONDS 8
  #define DISABLE_60_SECONDS 9
  #define DISABLE_120_SECONDS 10
  #define DISABLE_240_SECONDS 11
  #define DISABLE_480_SECONDS 12
  #define DISABLE_960_SECONDS 13
  #define DISABLE_1920_SECONDS 14
  #define DISABLE_3840_SECONDS 15
  
  public:
    d0gStep(uint8_t _maxMotors, uint8_t disableTime = DISABLE_30_SECONDS, uint8_t _en = 255, uint8_t _queueSize = 2);
/*
The constructor takes one parameter, which doesn't actually do anything in this version
TODO: make the maximum motor variable adjustable in the code 
PARAMETERS: The maximum number of motors, Idle time to disable motors, enable pin number, movement queue size
*/


    void printDebug();
/*
Prints debugging information over Serial
*/


    bool attachMotor(uint8_t _step, uint8_t _dir);
/*
Installs a motor driver
PARAMETERS: The motor's step pin, The motor's direction pin, The motor's enable pin
RETURNS: True if successful
*/


    int16_t currentSpeed(uint8_t _motor);
/*
Sets the movement speed of one motor
PARAMETERS: The motor number, The speed in steps per second
*/


    void moveMotor(uint8_t _motor, int16_t _speed);
/*
Causes a motor to move
PARAMETERS: The motor number, The motor speed (in steps per second) (negative speed makes the motor move backward)
*/


    void accelerate(uint8_t _motor, int16_t _speed, uint8_t multiplier = 64);
/*
Changes the motor's absolute speed
Does not affect unified motor motion
PARAMETERS: The motor number, The motor speed change (in steps per second) (negative decreases speed, positive increases spead)
*/


    uint8_t queues();

/*
RETURNS: number of movements queued
 */
 
    bool canQueue();
/*
Tests if queue buffer is full
RETURNS: True if a queue is possible, false if queue buffer is full
*/

      bool queueMotorsMovement(long _target[], float _time = 0.01);
/*
Sets the absolute target of all motors and time to arrive simultaneously
PARAMETERS: The targets, The time (in seconds) to arrive
RETURNS: True if successful, false if unable to add queue
*/

    bool interruptSoon();
/*
Checks if the interrupt is going to fire very soon
RETURNS: False if interrupt is not going to fire for some time, True if interrupt may fire soon
 */

    void stopMotor(uint8_t _motor);
/*
Stops one motor
PARAMETERS: The motor number
*/


    long currentPosition(uint8_t _motor);
/*
Gets the motor position
PARAMETERS: The motor number
RETURNS: the current position of a motor
*/


    long targetPosition(uint8_t _motor);
/*
Gets the motor's current target position
PARAMETERS: The motor number
RETURNS: the current target position of a motor
*/


    int32_t lastQueuedTarg(uint8_t _motor);
/*
Gets the motor's last target position
PARAMETERS: The motor number
RETURNS: the last target position of a motor
*/


    long distanceToGo(uint8_t _motor);
/*
Gets the distance to the motor's target
RETURNS: the distance from the current position to the target position of one motor
*/


    uint8_t moving(uint8_t _motor = 255);
/*
Gets the motor's status
PARAMETERS: The motor number
RETURNS: 1 if the motor is moving, 0 if not
*/


    void setMotorPosition(uint8_t _motor, long _position);
/*
Sets the current motor position and target position
The distance from the current position to the target position is maintained
PARAMETERS: The motor number, The desired motor position
*/


    uint16_t maxSpeed(uint8_t _motor, uint16_t _speed = 0);
/*
Sets the motor's maximum speed
PARAMETERS: The motor number, The desired max speed in steps per second
*/


    void setMotorMaxPosition(uint8_t _motor, long _position);
/*
Sets the maximum motor position
PARAMETERS: The motor number, The desired motor position
*/


    void setMotorMinPosition(uint8_t _motor, long _position);
/*
Sets the minimum motor position
PARAMETERS: The motor number, The desired motor position
*/


    void setMotorMinPositionHere(uint8_t _motor);
/*
Sets the minimum motor position at current position
PARAMETERS: The motor number
*/


    void setMotorMaxPositionHere(uint8_t _motor);
/*
Sets the maximum motor position at current position
PARAMETERS: The motor number
*/

    
    void getMotorMaxPosition(uint8_t _motor);
/*
Sets the maximum motor position
PARAMETERS: The motor number, The desired motor position
*/


    void getMotorMinPosition(uint8_t _motor);
/*
Sets the minimum motor position
PARAMETERS: The motor number, The desired motor position
*/


    void enable(uint8_t _e = 1);
/*
Enables all motors
PARAMETERS: 0 to pause all motors or anything else to enable all motors (optional)
*/


    void disable();
/*
Disables (pauses) all motors
*/


    bool nudge(uint8_t _motor, int _dist, float _speed);
/*
Moves one motor to a relative position at a set speed
Does not change the current position of the motor
Only works when the motors are disabled
Blocks during the move
PARAMETERS: The motor number, The distance (+ or -) to move, The speed in step per second
*/


    static void step();
/*
The stepper algorithm
*/    


    static void writeFast(uint8_t _pin, uint8_t _value);
/*
Pin state control
 */

 
    const long noMovement =  0x80000000;

/*
Path queuing will ignore this target (-2147483648)
Useful for causing motor to not move
 */

  private:

    const uint16_t maxSpeedLimit = 0x4FFF;
/*
Absolute maximum speed possible by library
 */


    unsigned long dyFactor = 2000000;
/*
Calculated factor for setting Bresenham Algorithm step rate
 */
  
  protected:
    static uint8_t maxMotors;
    static volatile uint8_t queueAvailable;
    static uint8_t motorCount;
    static uint8_t *pinDir; //https://stackoverflow.com/questions/751878/determine-array-size-in-constructor-initializer
    static uint8_t *pinStep;
    static uint8_t pinEn;
    static volatile long *pos;
    static volatile long *targ;
    static volatile unsigned long *dy2;
    static volatile uint16_t *maxStepPerSecond;
    volatile static unsigned int dx2; //Microseconds per call
    static unsigned int dx2Max;
    static unsigned int dx2Delta;
    static volatile long *D;
    static long *maxPos;
    static long *minPos; 
    static volatile uint16_t disableTimer;
    static volatile uint16_t maxTimeToStep;
    static volatile uint16_t minTimeToStep;
    static volatile uint32_t timeBetweenStep;
    static volatile uint8_t queuePos;
    static uint8_t queueSize;
    static uint8_t disableTimerFactor;
};
#endif
