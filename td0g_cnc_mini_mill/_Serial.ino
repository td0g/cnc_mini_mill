/////////////////////////////////////////////////////////

//                         Serial

/////////////////////////////////////////////////////////

void getCommand(){
  static byte crc;
  static byte doingSomething;
  static byte crcEnd = 0;
  static byte crcGood = 0;
  static unsigned long lineNumber = 0;
  static byte ignore = 0;
  switch (doingSomething){
    case 0:
      char c;
      c = 0;
      if (readFromSD){
        if (dataFile.available()) {
          c = dataFile.read();
          Serial.write(c);
          fileProg++;
        }
        else {
          endFileJob();
          lcd.setCursor(17, 0);
          lcd.print(F("DONE"));
        }
      }
      else if (Serial.available()) c = Serial.read();
      if(c == '\n') {
        lineNumber++;
        ignore = 0;
        if (serialBuffer[0] == ';') sofar = 0;      //Line is a comment
        else if (!readFromSD) doingSomething = 2;   //Execute line
        else doingSomething = 1;
      }
      else if(c != 0 && sofar < MAX_BUF) {
        if (!ignore) serialBuffer[sofar++] = c;
        if (c == ';') ignore = 1;
        else if (c == 42) crcEnd = 1;
        if (!crcEnd) crc ^= c;
      }
    break;
    case 1:
      if (!inputType && millis() > dwellTimer){
        if (readFromSD){
          float _t;
          _t = fileProg;
          _t /= fileSize;
          _t *= 100;
          while (!runLCD()){};
          lcd.setCursor(13, 0);
          lcd.print(_t);
          lcd.setCursor(19, 0);
          lcd.print(F("% "));
        }
        processCommand();
        doingSomething = 0;
        sofar = 0;
      }
    break;
    case 2:
        byte printedCRC;
        printedCRC = parseNumber(42, crc);
        if (crc == printedCRC){
          doingSomething = 1;
          crc = 0;
          crcEnd = 0;
          Serial.println(F("ok "));
        }
        else {
          while (Serial.available()) Serial.read();
          sofar = 0;
          Serial.print(F("N"));
          Serial.print(lineNumber);
          Serial.print(F(" Expected Checksum "));
          Serial.print(crc);
          Serial.print(F(" ("));
          Serial.print(printedCRC);
          Serial.println(F(")"));
          lineNumber--;
          doingSomething = 0;
        }
    break;
  }
}

void processCommand() {
  static float feedLast[2] = {FEEDRATE_DEFAULT,FEEDRATE_DEFAULT};
  static byte gZeroOne;
  byte done = 0;
  int cmd=parseNumber('G',-1);
  
  if (cmd > -1) done = 1;
  switch(cmd) {
    case 0: case 1: gZeroOne = cmd; done = 0; break; // move in a line - this is done below     
    case 4: dwellTimer = millis() + parseNumber('P',0) * 1000; break; // wait a while
    case 29: Serial.print(F("X: ")); Serial.print(getMM(0)); Serial.print(F(" Y: ")); Serial.print(getMM(1));
      Serial.print(F(" Z: ")); Serial.println(zProbe(parseNumber('Z', probePlunge))); done = 1;  break;
    case 90: posABS=1; left += 4; processCommand(); break; // absolute mode
    case 91: posABS=0; left += 4; processCommand();  break; // relative mode
    case 92: 
      for (byte i = 0; i < 3; i++)xyzMotors[i]->setCurrentPosition(parseNumber(axisLetter[i], getMM(i)) * stepPerMM[i]);
    break;
  }

  if (!done){
    cmd=parseNumber('M',-1);
    if (cmd > -1) done = 1;
    switch(cmd) {
      case 3: tool(1); break;
      case 5: tool(0); break;
      case 18: break; // turns off power to steppers (releases the grip)
      case 105: Serial.println(F("T:0 B:0")); break;
      case 114: case 119:
        for (byte i = 0; i < 3; i++){
          Serial.print(axisLetter[i]);
          Serial.print(F(":"));
          if (cmd == 114) Serial.print(getMM(i));
          else Serial.print((endstopState >> i) & 1);
          Serial.print(F(" "));
        }
      break;
    }
  }
  
  if (!done){
    float _xyz[3];
    for (byte i = 0; i < 3; i++){
      if (posABS) _xyz[i] = parseNumber(axisLetter[i], getMM(i));
      else _xyz[i] = parseNumber(axisLetter[i], 0) + getMM(i);
    }
    feedLast[gZeroOne] = parseNumber('F',feedLast[gZeroOne]);
    lineAbs(_xyz, feedLast[gZeroOne]);
  }
}



float parseNumber(char code,float val) {
  char *ptr=serialBuffer + left;  // start at the beginning of buffer
  while((long)ptr > 1 && (*ptr) && (long)ptr < (long)serialBuffer+sofar) {  // walk to the end
    if(*ptr==code) {  // if you find code on your walk,
      return atof(ptr+1);  // convert the digits that follow into a float and return it
    }
    ptr++;  // take a step from here to the letter after the next space
  }
  return val;  // end reached, nothing found, return default val.
}
