/////////////////////////////////////////////////////////

//                         Screen

/////////////////////////////////////////////////////////

void printScreen(){
  static byte _menuPosition;
  if (menuPosition != _menuPosition){
    menuPosition = menuPosition & 0b01111111;
    while (!runLCD()){};
    if (!(menuPosition >> 2) || (menuPosition >> 2) != (_menuPosition >> 2)) lcd.clear();
    for (byte i = 0; i < 4; i++){
      lcd.setCursor(0, i);
      if (i == (menuPosition & 0b00000011)) lcd.print(F(">"));
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
        lcd.print(F("ZProbe Single/Grid")); //8
        lcd.setCursor(1, 1);
        lcd.print(F("-")); //9
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
        for (byte i = 0; i < 3; i++){ //20, 21, 22
          lcd.setCursor(1, i);
          lcd.print(axisLetter[i]);
          lcd.print(F("stps "));
          lcd.print(axisSize[i]);
          lcd.print(F(" / "));
          lcd.print(xyzMotors[i]->currentPosition());
        }
      break;
      case 6:
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
            y.stop();
            z.stop();
          }
        }
        else {
          menuPosition = 4;
          if (inputType) setPause(1);
        }
      break;
      case 4: tool(2); break;
      case 5: 
        if (btn == 2) {x.setCurrentPosition(0); y.setCurrentPosition(0); menuPosition = 2; }
        z.setCurrentPosition(0);
      break;
      case 6: 
        if (!inputType){
          lcd.clear();
          if (cardAvailable) SD.end();
          cardAvailable = SD.begin(4);
          PRINT_LCD;
          if (cardAvailable){menuPosition = 12; lcd.clear(); printDirectory();} 
        }
        else if (readFromSD) {
          endFileJob();
          x.move(0);
          y.move(0);
          z.move(0);
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
          printScreen();
          lcd.setCursor(15, 1); 
          if (_z == 9999) lcd.print(F("Err")); 
          else printFltNB(16, 1, _z, 2, 7);
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
      case 10:
        menuPosition = 16;
      break;
      case 12: case 13: case 14: case 15: openFileIndex(fileIndex[menuPosition - 12]); break;
      case 26:   
        EEPROMWriteint(0, 0);
        resetFunc(); //call reset 
      break;
      case 19: case 23: case 27: exportSettings(); menuPosition = 10; break;
      default: menuPosition = 2; break;
    }
  }
}



void printPositionscreen(){
  static byte rowXYZ;
  if (runLCD()){
    rowXYZ = (rowXYZ + 1) % 3;
    printFltNB(10, rowXYZ + 1, getMM(rowXYZ), 2, 7);
  }
}

uint8_t readBtn(){  //Return 1 if short pressed & released, Return 2 if long pressed PRIOR to release
  static unsigned long debounceTimer;
  static byte buttonRead;
  static byte buttonPos;
  
  buttonRead = digitalRead(CONTROL_CLICK_PIN);  //For pullup input
  if (!buttonRead && millis() > debounceTimer){   //Button depressed
    if (!buttonPos){buttonPos = 1;debounceTimer = millis() + BUTTON_LONGHOLD;}
    else if (buttonPos == 1){buttonPos = 2; return 2;}
  }
  else if (buttonPos && buttonRead) {debounceTimer = millis() + BUTTON_DEBOUNCE;
    if (buttonPos == 1) {buttonPos = 0; return 1;}
    buttonPos = 0;
  }
  return 0;
}
