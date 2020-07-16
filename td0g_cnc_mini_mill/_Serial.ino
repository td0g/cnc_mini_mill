/////////////////////////////////////////////////////////

//                         Serial

/////////////////////////////////////////////////////////

void getCommand(){
  if (sofar && serialBuffer[sofar - 1] == 10) {
    if (serialBuffer[0] == ';') sofar = 0;      //Line is a comment
    else if (millis() > dwellTimer){
      serialBuffer[sofar] = 0;  //VERY rare bug where parseNumber doesn't know where to stop without this line - adds a terminator
      if ((isCommandMovement() && motors.canQueue()) || !motors.moving()){
        if (processCommand()) sofar = 0;
      }
    }
  }
  else {
    char c = 0;
    if (externalInputMovement == 2){
      if (dataFile.available()) c = dataFile.read();
      else if (sofar) c = 10;
      else if (!motors.moving()) endFileJob();
    }
    else if (Serial.available()) c = Serial.read();
    if (c){
      Serial.write(c);
      if (c == 13) c = 10;  //Convert carriage return to newline
      else if (c > 96 && c < 123) c -= 32; //Convert to uppercase
      if (!sofar || c == 10) {
        serialBuffer[sofar] = c;
        sofar++;
      }
      else if(sofar < MAX_BUF && serialBuffer[sofar-1] != ';' && serialBuffer[sofar-1] != '(') {
        serialBuffer[sofar] = c;
        sofar++;
      }
      if (c == 10) fileProg++;
    }
  }
  TEST_FREE_MEMORY;
}

bool isCommandMovement(){
  int8_t g = parseNumber('G',-1);
  if (g == 0 || g == 1) return true;  //Is a G0 or G1
  else if (g == -1 && parseNumber('M',-1) == -1) return true; //Is a movement without G0/G1 (eg. X..Y..Z..)
  return false;
}


bool processCommand() {
  static float feedLast[2] = {FEEDRATE_DEFAULT_G0,FEEDRATE_DEFAULT_G1};
  static uint8_t gZeroOne;
  static uint8_t posABS = 1;
  int8_t cmd=parseNumber('G',-1);

  switch(cmd) {
    case 0: case 1: gZeroOne = cmd; cmd = -1; break; // move in a line - this is done below     
    case 4: dwellTimer = millis() + parseNumber('P',0) * 1000; break; // wait a while
    case 29: Serial.print(F("X: ")); Serial.print(getMM(0)); Serial.print(F(" Y: ")); Serial.print(getMM(1));
      Serial.print(F(" Z: ")); Serial.println(zProbe(false));  break;
    case 90: posABS=1; break; // absolute mode
    case 91: posABS=0; break; // relative mode
    case 92: 
      for (byte i = 0; i < 3; i++)motors.setMotorPosition(i, parseNumber(axisLetter[i], getMM(i)) * stepPerMM[i]);
    break;
  }

  if (cmd == -1){
    cmd=parseNumber('M',-1);
    switch(cmd) {
      case 3: tool(1); break;
      case 5: tool(0); break;
      case 105: Serial.println(F("T:0 B:0")); break;
      case 114: case 119:
        for (byte i = 0; i < 3; i++){
          Serial.print(axisLetter[i]);
          Serial.print(F(":"));
          if (cmd == 114) Serial.print(getMM(i));
          else Serial.print((globalEndstopState >> i) & 1);
          Serial.print(F(" "));
        }
      break;
      case 120: endstopMask = 0b11111111; break;
      case 121: endstopMask = 0b10000000; break;
    }
  }

  //Is a movement
  if (cmd == -1){
    float _timeInSeconds = 0;
    long xyzInt[3];
    float _s;
    for (byte i = 0; i < 3; i++){
      float _parse = parseNumber(axisLetter[i], 10000);
      if (_parse > 1000) xyzInt[i] = motors.noMovement;
      else {
        xyzInt[i] = stepPerMM[i] * _parse;
        if (!posABS) xyzInt[i] += motors.lastQueuedTarg(i);
        else {
          _s = motors.lastQueuedTarg(i);
          _s /= stepPerMM[i];
          _parse -= _s;
        }
        _timeInSeconds = _timeInSeconds + _parse * _parse;
      }
    }
    _timeInSeconds = sqrt(_timeInSeconds);
    
    _s = parseNumber('F',feedLast[gZeroOne]);
    if (_s > 0) feedLast[gZeroOne] = _s;
    else _s = feedLast[gZeroOne];
    _s *= speedMultiplier;
    _s /= 100;
    
    _timeInSeconds = _timeInSeconds * 60 / _s;
    TEST_FREE_MEMORY;
    if (motors.queueMotorsMovement(xyzInt, _timeInSeconds)){
      if (!externalInputMovement){
        externalInputMovement = 1;
        PRINT_LCD;
      }
      menuPosition = 1;
      return true;
    }
    return false;
  }
  TEST_FREE_MEMORY;
  return true;
}



float parseNumber(char code,float val) {
  char *ptr=serialBuffer;  // start at the beginning of buffer
  while((long)ptr > 1 && (*ptr) && (long)ptr < (long)serialBuffer+sofar) {  // walk to the end
    if(*ptr==code) {  // if you find code on your walk,
      return atof(ptr+1);  // convert the digits that follow into a float and return it
    }
    ptr++;  // take a step from here to the letter after the next space
  }
  return val;  // end reached, nothing found, return default val.
}
