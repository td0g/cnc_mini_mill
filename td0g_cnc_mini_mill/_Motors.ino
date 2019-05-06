boolean runMotors(){
  static byte _lastEndstopstate = endstopState;
  byte _running = 0;
  if (pause) return true;
  for (byte i = 0; i < 3; i++){
    if (inputType) {
      xyzMotors[i]->runSpeedToPosition();
      if (xyzMotors[i]->distanceToGo() != 0) _running = 1;
    }
    else _running |= xyzMotors[i] -> run();
    if (!(_lastEndstopstate & (1 << i)) && (endstopState & (1 << i))){ //run algorithm on RISING EDGE
      if (xyzMotors[i]->distanceToGo() > 0){  //Moving ++
        if (minmax[i][0] < xyzMotors[i]->currentPosition() - 1000) {
          if (minmax[i][1] - 1000 > xyzMotors[i]->currentPosition()) {  //We are not close to the expected endstop position - set new endstop position
            minmax[i][1] = xyzMotors[i]->currentPosition();
            minmax[i][0] = minmax[i][1] - axisSize[i];
          }
          xyzMotors[i]->move(0);
          xyzMotors[i]->setSpeed(0);
        }
      }
      else {  //moving--;
        if (minmax[i][1] - 1000 > xyzMotors[i]->currentPosition()){
          if (xyzMotors[i]->currentPosition() - 1000 > minmax[i][0]){  //We are not close to the expected endstop position - set new endstop position
            minmax[i][0] = xyzMotors[i]->currentPosition();
            minmax[i][1] = minmax[i][0] + axisSize[i];
          }
          xyzMotors[i]->move(0);
          xyzMotors[i]->setSpeed(0);
        }
      }
    }
  }
  if (inputType && !_running) {
    inputType = 0;
    x.setSpeed(0);
    y.setSpeed(0);
    z.setSpeed(0);
  }
  _lastEndstopstate = endstopState;
  return _running;
}

float getMM(byte axis){
  float temp;
  temp = xyzMotors[axis]->currentPosition();
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
    if (xyzStep != xyzMotors[i]->currentPosition()) {
      xyzMotors[i]->moveTo(xyzStep);
      if (motorCount == 4) motorCount = i;
      else motorCount = 255;
    }
  }
//Move only one motor?
  if (motorCount < 4) {
    inputType = 1;
    temp = _feedRate * stepPerMM[motorCount] / 60;
    temp = min(temp, stepRateMax[motorCount]);
    xyzMotors[motorCount]->setMaxSpeed(temp);
    xyzMotors[motorCount]->setSpeed(temp);
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
      temp = abs(xyzMotors[i]->distanceToGo());
      temp /= stepRateMax[i];
      if (temp > timeInSeconds) timeInSeconds = temp;
    }
  //Set step rate for each motor
    for (byte i = 0; i < 3; i++){
      temp = abs(xyzMotors[i]->distanceToGo());
      if (temp != 0) {
        temp /= timeInSeconds;
        xyzMotors[i]->setMaxSpeed(temp);
        xyzMotors[i]->setSpeed(temp);
        if (xyzMotors[i]->targetPosition() > minmax[i][1]) xyzMotors[i]->moveTo(minmax[i][1]);
        else if (xyzMotors[i]->targetPosition() < minmax[i][0]) xyzMotors[i]->moveTo(minmax[i][0]);
      }
    }
  }
}
