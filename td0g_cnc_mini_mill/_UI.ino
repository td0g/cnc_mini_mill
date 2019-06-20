/////////////////////////////////////////////////////////

//                         Screen

/////////////////////////////////////////////////////////

void printScreen(){
  static byte _menuPosition;
  if (menuPosition != _menuPosition){
    menuPosition = menuPosition & 0b01111111;
    //positionCharQueue = 0;
    if (!(menuPosition >> 2) || (menuPosition >> 2) != (_menuPosition >> 2)) lcd.clear();
    for (byte i = 0; i < 4; i++){
      lcd.setCursor(0, i);
      if (i == (menuPosition & 0b00000011) && menuPosition > 1) lcd.print(F(">"));
      else lcd.print(F(" "));
    }
    _menuPosition = menuPosition >> 2;
    lcd.setCursor(1, 3);
    switch (_menuPosition){
      case 1: case 2: lcd.print(F("Exit")); break;
      case 4: case 5: case 6: lcd.print(F("Save")); break;
    }
    lcd.setCursor(1, 0);
    switch (_menuPosition){
      case 0:
        lcd.setCursor(0, 0);
        if (!dataFile) lcd.print(F("CNC MINI MILL")); 
        else lcd.print(dataFile.name());
        if (pause){
          lcd.setCursor(13, 0);
          lcd.print(F("    -P-"));
        }
        else {
          lcd.setCursor(14, 0);
          lcd.print(F("   "));
          if (readFromSD) lcd.print(F(" % "));
        }
        for (byte i = 0; i < 3; i++){
          lcd.setCursor(1, i + 1);
          lcd.print(axisLetter[i]);
          lcd.print(F(":         mm"));
        }
      break;
      case 1:
        lcd.print(F("Tool")); //4
        lcd.setCursor(1, 1);
        lcd.print(F("Set Z=0 / Origin"));  //5
        lcd.setCursor(1, 2);                  //6
        if (inputType) lcd.print(F("End Job"));
        else if (cardAvailable) lcd.print(F("SD Files  ")); 
        else lcd.print(F("Check Card")); 
      break;
      case 2:
        lcd.print(F("ZProbe "));
        if (!motors.currentPosition(0) && !motors.currentPosition(1) && motors.currentPosition(2))lcd.print(F("Zero"));//!x.currentPosition() && !y.currentPosition()) lcd.print(F("Zero")); //8
        else lcd.print(F("Single")); //8
        lcd.print(F("/Grid"));
        lcd.setCursor(1, 1);
        lcd.print(F("Goto0,0")); //9
        lcd.setCursor(1, 2);
        lcd.print(F("Settings")); //10
      break;
      case 4:
        for (byte i = 0; i < 3; i++){ //16, 17, 18
          lcd.setCursor(1, i);
          lcd.print(axisLetter[i]);
          lcd.print(F(" MxSpd "));
          lcd.print(stepRateMax[i]);
        }
      break;
      case 5:
        lcd.print(F("ProbeGridMM ")); //24
        lcd.print(probeGridDist);
        lcd.print(F("  "));
        lcd.setCursor(1, 1);  //25
        lcd.print(F("ProbePlungeMM "));
        lcd.print(probePlunge);
        lcd.setCursor(1, 2);
        lcd.print(F("Load Defaults"));  //26
      break;
    }
    _menuPosition = menuPosition;
  }
}

void runUI(){
  byte btn = readBtn();
  if (btn){
    switch (menuPosition){
      case 2: case 3:
        if (btn == 1) {
          if (inputType){
            setPause(2);
          }
          else {
            menuPosition = 5 - menuPosition;
            motors.stopOneMotor(1);//y.stop();
            motors.stopOneMotor(2);//z.stop();
          }
        }
        else {
          menuPosition = 4;
          if (inputType) setPause(1);
        }
      break;
      case 4: tool(2); break;
      case 5: 
        if (btn == 2) {motors.setCurrentPosition(0, 0); motors.setCurrentPosition(1, 0); menuPosition = 2;}//x.setCurrentPosition(0); y.setCurrentPosition(0); menuPosition = 2; }
        motors.setCurrentPosition(2, 0);//z.setCurrentPosition(0);
      break;
      case 6: 
        if (!inputType){
          lcd.clear();
          if (cardAvailable) SD.end();
          cardAvailable = SD.begin(4);
          PRINT_LCD;
          if (cardAvailable){menuPosition = 12; printScreen(); printDirectory();} 
        }
        else if (readFromSD) {
          endFileJob();
        }
      break;
      case 8:
        if (btn == 1) {
          menuPosition = 2;
          PRINT_LCD;
          printScreen();
          float _z; _z = zProbe(probePlunge);  
          menuPosition = 11;
          PRINT_LCD;
          if (!motors.currentPosition(0) && !motors.currentPosition(1)){//!x.currentPosition() && !y.currentPosition()){
            //z.setCurrentPosition(getMM(2)-_z*stepPerMM[2]);
            motors.setCurrentPosition(2, getMM(2) - _z*stepPerMM[2]);
          }
          else {
            printScreen();
            lcd.setCursor(15, 1); 
            if (_z == 9999) lcd.print(F("Err")); 
            else {
              lcd.setCursor(17, 1);
              lcd.print(_z);
            }
            //else printFltNB(17, 1, _z);
          }
        }
        else if (cardAvailable) {
          menuPosition = 2;
          PRINT_LCD;
          printScreen();
          probeGrid();
          menuPosition = 9;
          PRINT_LCD;
          printScreen();
        }
      break;
      //case 9: x.moveTo(0); y.moveTo(0); x.setMaxSpeed(stepRateMax[0] * 2); y.setMaxSpeed(stepRateMax[1] * 2); menuPosition = 2; break;
      case 9: motors.moveOneMotorTo(0,0); motors.moveOneMotorTo(1,0); motors.setOneSpeed(0, stepRateMax[0]*2);motors.setOneSpeed(1, stepRateMax[1]*2); menuPosition = 2; break;
      case 10:
        menuPosition = 16;
      break;
      case 12: case 13: case 14: case 15:
        openFileIndex(fileIndex[menuPosition - 12]);
      break;
      case 26:   
        EEPROMWriteint(0, 0);
        resetFunc(); //call reset 
      break;
      case 19: case 23: case 27: exportSettings(); menuPosition = 10; break;
      default: menuPosition = 2; break;
    }
  }
}


uint8_t readBtn(){  //Return 1 if short pressed & released, Return 2 if long pressed PRIOR to release
  static unsigned long debounceTimer;
  static byte buttonPos;
  
  if (!digitalRead(CONTROL_CLICK_PIN) && millis() > debounceTimer){   //Button depressed
    if (!buttonPos){buttonPos = 1; debounceTimer = millis() + BUTTON_LONGHOLD;}
    else if (buttonPos == 1){buttonPos = 2; return 2;}
  }
  else if (buttonPos && digitalRead(CONTROL_CLICK_PIN)) {debounceTimer = millis() + BUTTON_DEBOUNCE;
    if (buttonPos == 1) {buttonPos = 0; return 1;}
    buttonPos = 0;
  }
  return 0;
}
