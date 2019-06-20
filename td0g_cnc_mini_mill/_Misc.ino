

/////////////////////////////////////////////////////////

//                         Misc

/////////////////////////////////////////////////////////

void(* resetFunc) (void) = 0;//declare reset function at address 0

/////////////////////////////////////////////////////////

//                         Tool

/////////////////////////////////////////////////////////

void tool(byte input){
  if (input > 1) tool(digitalRead(TOOL_PIN));
  else digitalWrite(TOOL_PIN, 1 - input);
}


void setPause(byte p){
  if (p == 2) pause = 1 -pause;
  else pause = min(p, 1);
  PRINT_LCD;
}

/////////////////////////////////////////////////////////

//                         Probing

/////////////////////////////////////////////////////////



float zProbe(float probedist){
  unsigned long startZ = motors.currentPosition(2);//z.currentPosition();
  motors.setOneSpeed(2, 0.4 * stepPerMM[2]);
  motors.moveOneMotor(2, fabs(probedist) * stepPerMM[2] * -1);
  //z.move(fabs(probedist) * stepPerMM[2] * -1);
  //z.setSpeed(0.4 * stepPerMM[2]);
  inputType = 1;
  while (!(endstopState & 0b10000000) && motors.distanceToGo(2)){//runMotors()){
    analogReadAll();
    printPositionscreen();
  }
  float _z;
  if (endstopState & 0b10000000)_z = getMM(2);
  else _z = 9999;
  inputType = 0;
  
  //z.setSpeed(0);
  motors.moveOneMotor(2, startZ);//z.setMaxSpeed(stepPerMM[2]);
  motors.setOneSpeed(2, stepPerMM[2]);//z.moveTo(startZ);
  while (motors.distanceToGo(2)){//runMotors()){
    printPositionscreen();
    analogReadAll();
  }  
  return _z;
}

void probeGrid(){
//Prepare file and screen
  dataFile = SD.open("probe.txt", FILE_WRITE);
  if (!dataFile){
    cardAvailable = 0;
    return;
  }
  dataFile.println(F("X,Y,Z"));
  PRINT_LCD;
  printScreen();

//Prepare variables
  float _xyMovementSize[2];
  float _targetxyz[3];
  byte _xyDivisors[2];

//Calculate number of probes
  for (byte i = 0; i < 2; i++){
    motors.setOneSpeed(i, stepRateMax[i]*2);
    //xyzMotors[i]->setMaxSpeed(stepRateMax[i] * 2);
    _xyMovementSize[i] = getMM(i);
    _xyDivisors[i] = 1;
    if (motors.currentPosition(i)){//xyzMotors[i]->currentPosition()){
      while ((float)_xyDivisors[i] * probeGridDist < fabs(_xyMovementSize[i])) _xyDivisors[i]++;
      _xyMovementSize[i] /= _xyDivisors[i];
      _xyDivisors[i]++;
    }
    _targetxyz[i] = getMM(i);
  }

//Plan route
  byte shortAxis;
  byte longAxis;
  if (fabs(_xyMovementSize[0]) > fabs(_xyMovementSize[1])) longAxis = 0;
  else longAxis = 1;
  shortAxis = 1 - longAxis;

//Execute
  _targetxyz[2] = getMM(2);
  _targetxyz[longAxis] += _xyMovementSize[longAxis];
  for (byte i = 0; i < _xyDivisors[longAxis]; i++){
    _targetxyz[longAxis] -= _xyMovementSize[longAxis];
    _targetxyz[shortAxis] += _xyMovementSize[shortAxis];
    _xyMovementSize[shortAxis] *= -1;
    for (byte j = 0; j < _xyDivisors[shortAxis]; j++){
      _targetxyz[shortAxis] += _xyMovementSize[shortAxis];
      lineAbs(_targetxyz, 600);
      inputType = 0;
      while (motors.distanceToGo(2)){//runMotors()){
        printPositionscreen();
        analogReadAll();
      }
      dataFile.print(getMM(0));
      dataFile.print(F(","));
      dataFile.print(getMM(1));
      dataFile.print(F(","));
      dataFile.println(zProbe(probePlunge));
    }
  }
  dataFile.close();
  PRINT_LCD;
}
