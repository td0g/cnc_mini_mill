void endstopHit(){
  for (byte i = 0; i < 3; i++){
    if (motors.distanceToGo(i)){
      if (motors.distanceToGo(i) > 0){
        if ((endstopState >> (i+3)) & 1) motors.stopOneMotor(i);
      }
      else {
        if ((endstopState >> (i)) & 1) motors.stopOneMotor(i);
      }
    }
  }
}

float getMM(byte axis){
  float temp;
  temp = motors.currentPosition(axis);//xyzMotors[axis]->currentPosition();
  temp /= stepPerMM[axis];
  return temp;
}

void lineAbs(float _xyz[3], float _feedRate){
  //Draws a straight line from current position
  float temp;
  byte motorCount = 4;
  
//Figure out how many motors are moving
  for (byte i = 0; i < 3; i++){
    temp = _xyz[i] * stepPerMM[i];
    long xyzStep;
    xyzStep = temp;
    //if (xyzStep != xyzMotors[i]->currentPosition()) {
    if (xyzStep != motors.currentPosition(i)) {
      motors.moveOneMotorTo(i, xyzStep);//xyzMotors[i]->moveTo(xyzStep);
      if (motorCount == 4) motorCount = i;
      else motorCount = 255;
    }
  }
//Move only one motor?
  if (motorCount < 4) {
    inputType = 1;
    temp = _feedRate * stepPerMM[motorCount] / 60;
    temp = min(temp, stepRateMax[motorCount]);
    motors.setOneSpeed(motorCount, temp);//xyzMotors[motorCount]->setMaxSpeed(temp);
    //xyzMotors[motorCount]->setSpeed(temp);
  }
//Move multiple motors?
  else if (motorCount == 255) {
    inputType = 1;
  //Get time to move to destination
    float timeInSeconds = 0;
    for (byte i = 0; i < 3; i++){
      _xyz[i] -= getMM(i);
      _xyz[i] = fabs(_xyz[i]);
      timeInSeconds = timeInSeconds + sq(_xyz[i]);
    }
    timeInSeconds = sqrt(timeInSeconds);
    timeInSeconds *= 60;                        //Convert to mm/s
    timeInSeconds = timeInSeconds / _feedRate; //time in seconds;
  //Get minimum time for each axis to complete, make sure total time is not less than that
    for (byte i = 0; i < 3; i++){
      temp = abs(motors.distanceToGo(i));//abs(xyzMotors[i]->distanceToGo());
      temp /= stepRateMax[i];
      if (temp > timeInSeconds) timeInSeconds = temp;
    }
  //Set step rate for each motor
    for (byte i = 0; i < 3; i++){
      temp = abs(motors.distanceToGo(i));//abs(xyzMotors[i]->distanceToGo());
      if (temp != 0) {
        temp /= timeInSeconds;
        motors.setOneSpeed(i, temp);//xyzMotors[i]->setMaxSpeed(temp);
        //xyzMotors[i]->setSpeed(temp);
      }
    }
  }
}
