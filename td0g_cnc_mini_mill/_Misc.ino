/////////////////////////////////////////////////////////

//                         Misc

/////////////////////////////////////////////////////////

void(* resetFunc) (void) = 0;//declare reset function at address 0


float getMM(byte axis){
  float temp;
  temp = motors.currentPosition(axis);
  temp /= stepPerMM[axis];
  return temp;
}


void runMacros(){
  if (externalInputMovement & 0b11111100){
    if (externalInputMovement == 4) probeGrid();
    else if (externalInputMovement == 5 || externalInputMovement == 6) {
      long xyz[3] = {0, 0, motors.noMovement};
      if (externalInputMovement == 6) xyz[2] = 0;
      motors.queueMotorsMovement(xyz);
    }
  externalInputMovement = 0;
  }
}

/////////////////////////////////////////////////////////

//                  Tool, Pause, Endstop

/////////////////////////////////////////////////////////

void tool(byte input){
  if (input & 0b11111110) tool(digitalRead(TOOL_PIN));
  else digitalWrite(TOOL_PIN, 1 - input);
}


void setPause(byte p){
  if (p & 0b11111110) pause = 1 -pause;
  else pause = p;
  motors.enable(1-pause);
}


void setEndstop(byte e){
  if (!e || e == 0b11111111) endstopMask = 0b10000000;
  else if (e == 1 || e == 0b10000000) endstopMask = 0b11111111;
}


/////////////////////////////////////////////////////////

//                         Probing

/////////////////////////////////////////////////////////



float zProbe(bool _setZero){
  float _z = 9999;
  long xyz[3] = {motors.noMovement, motors.noMovement, 0};
  delay(200);
  xyz[2] = motors.currentPosition(2) - stepPerMM[2] * probePlunge / 10;
  motors.queueMotorsMovement(xyz, 14);
  while (!(globalEndstopState & 0b10000000) && motors.moving() && digitalRead(CONTROL_CLICK_PIN)){
    analogReadAll();
    printPositionscreen();
    //wdt_reset();
  }
  motors.stopMotor(2);
  delay(25);
  if (globalEndstopState & 0b10000000) {
    if (_setZero) motors.setMotorPosition(2,0);
    _z = getMM(2);
  }
  xyz[2] = motors.currentPosition(2) + stepPerMM[2] * probeReturn / 10;
  motors.queueMotorsMovement(xyz);
  globalEndstopState &= 0b01111111;
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
  //PRINT_LCD;
  //printScreen();

//Prepare variables
  int _xyMovementSize[2];
  long _targetxyz[3] = {0, 0, motors.noMovement};
  byte _xyDivisors[2] = {0, 0};
  int _probesLeft;
  float _z;

//Calculate number of probes
  for (byte i = 0; i < 2; i++){
    //_xyDivisors[i] = 1;
    //while (abs(motors.currentPosition(i)) > _xyDivisors[i] * probeGridDist * stepPerMM[i]) _xyDivisors[i]++;
    _xyDivisors[i] = abs(motors.currentPosition(i)) / (probeGridDist * stepPerMM[i]) + 1;
    _xyMovementSize[i] = motors.currentPosition(i) / _xyDivisors[i];
    _targetxyz[i] = (long)_xyMovementSize[i]*_xyDivisors[i];
    _xyDivisors[i]++;
    TEST_FREE_MEMORY;
  }

//Plan route
  byte longAxis = 1;
  if (abs(_xyMovementSize[0]) > abs(_xyMovementSize[1])) longAxis = 0;
  byte shortAxis = 1 - longAxis;
  _probesLeft = (int)_xyDivisors[0] * _xyDivisors[1];
  
//Execute
  while (!digitalRead(CONTROL_CLICK_PIN)){};
  delay(100);
  while (_probesLeft){
    _probesLeft--;
    motors.queueMotorsMovement(_targetxyz);
    lcd.setCursor(17, 0);
    lcd.print(_probesLeft);
    lcd.print(F(" "));
    _z = zProbe(false);
    if (!digitalRead(CONTROL_CLICK_PIN)) _probesLeft = 0;
    for (byte i = 0; i < 2; i++){
      dataFile.print(getMM(i));
      dataFile.print(F(","));
      _targetxyz[i] += dither[i];
    }
    motors.queueMotorsMovement(_targetxyz);
    _targetxyz[shortAxis] -= dither[shortAxis];
    _targetxyz[longAxis] -= dither[longAxis];
    _z += zProbe(false);
    if (!digitalRead(CONTROL_CLICK_PIN)) _probesLeft = 0;
    _z /= 2;
    dataFile.println(_z, 3);
    if (!(_probesLeft % _xyDivisors[shortAxis])){
      _xyMovementSize[shortAxis] *= -1;
      _targetxyz[longAxis] -= _xyMovementSize[longAxis];
    }
    else _targetxyz[shortAxis] -= _xyMovementSize[shortAxis];
  }
  dataFile.close();
  PRINT_LCD;
}
