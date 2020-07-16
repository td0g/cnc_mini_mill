/////////////////////////////////////////////////////////

//                         Screen

/////////////////////////////////////////////////////////


#define MENU_MOVE_XY 2
#define MENU_MOVE_XZ 3
#define MENU_TOOL_ON_OFF 4
#define MENU_SET_POSITION_ZERO 5
#define MENU_SD 6
#define MENU_EXIT_OR_SETTINGS 7
#define MENU_Z_PROBE 8
#define MENU_MOVE_XY_HOME 9
#define MENU_DISABLE_STEPPERS_ENDSTOP 10
#define MENU_SAVE_RECALL_POSITION 11
#define MENU_SD_FILE_1 12
#define MENU_SD_FILE_2 13
#define MENU_SD_FILE_3 14
#define MENU_SD_FILE_4 15
#define MENU_SETTINGS 16
#define MENU_SETTINGS_EXIT_1 19
#define MENU_SETTINGS_PROBE_GRID 20
#define MENU_SETTINGS_PROBE_PLUNGE 21
#define MENU_SETTINGS_PROBE_RETURN 22
#define MENU_SETTINGS_EXIT_2 23
#define MENU_SETTINGS_LOAD_DEFAULT 24

void printScreen(){
  static byte _menuPosition;
  if (menuPosition != _menuPosition){
    if (!(menuPosition & 0b11111100) || (menuPosition & 0b11111100) != (_menuPosition & 0b11111100)) lcd.clear();
    for (byte i = 0; i < 4; i++){
      lcd.setCursor(0, i);
      if (i == (menuPosition & 0b00000011) && menuPosition > 1) lcd.print(F(">"));
      else lcd.print(F(" "));
    }
    if (!(menuPosition & 0b11111100) || (menuPosition & 0b11111100) != (_menuPosition & 0b11111100)){
      lcd.setCursor(1,0);
      _menuPosition = (menuPosition & 0b01111111) >> 2;
      switch (_menuPosition){
        case 0:
          lcd.setCursor(0, 0);
          if (!dataFile) lcd.print(F("CNC MINI MILL")); 
          else lcd.print(dataFile.name());
          if (externalInputMovement){
            lcd.setCursor(16,3);
            lcd.print(speedMultiplier);
            lcd.setCursor(19,3);
            lcd.print(F("%"));
            lcd.setCursor(14, 0);
            lcd.print(F("   "));
            if (externalInputMovement == 2) lcd.print(F(" % "));
            lcd.setCursor(13,2);
            if (menuPosition == 2) lcd.print(F(" /TOOL"));
            else if (menuPosition == 3) lcd.print(F(" / END"));
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
          if (cardAvailable) lcd.print(F("SD Files  ")); 
          else lcd.print(F("Check Card")); 
          lcd.setCursor(1, 3);
          lcd.print(F("Exit / Settings")); break;
        break;
        case 2:
          lcd.print(F("ZProbe Single/Grid"));
          lcd.setCursor(1, 1);
          lcd.print(F("Goto0,0,(0)")); //9
          lcd.setCursor(1, 2);
          lcd.print(F("Endstop O"));
          if (endstopMask & 0b01111111) lcd.print(F("n "));
          else lcd.print(F("ff"));
          lcd.setCursor(1,3);
          lcd.print(F("Save / Load Pos"));
        break;
        case 3:
          printDirectory();
        break;
        case 4:
          for (byte i = 0; i < 3; i++){ //16, 17, 18
            lcd.setCursor(1, i);
            lcd.print(axisLetter[i]);
            lcd.print(F(" MxSpd "));
            lcd.print(motors.maxSpeed(i));
          }
          lcd.setCursor(1,3);
          lcd.print(F("Save"));
        break;
        case 5:
          lcd.print(F("GridMM ")); //24
          lcd.print(probeGridDist);
          lcd.print(F("  PROBE"));
          lcd.setCursor(1, 1);  //25
          lcd.print(F("PlungeMM "));
          lcd.print(0.1 * probePlunge);
          lcd.setCursor(1, 2);  //26
          lcd.print(F("ReturnMM "));
          lcd.print(0.1 * probeReturn);
          lcd.setCursor(1,3);
          lcd.print(F("Save"));
        break;
        case 6:
          lcd.setCursor(1, 0);
          lcd.print(F("Load Defaults"));  //30
        break;
      }
    }
    menuPosition = menuPosition & 0b01111111;
    _menuPosition = menuPosition;
    TEST_FREE_MEMORY;
  }
}

void runUI(){
  if (motors.interruptSoon()) return;
  byte btn = readBtn();
  if (btn){
    if (externalInputMovement){
        setPause(1);      
        if (menuPosition == MENU_MOVE_XY && btn == 2) tool(2);
        else if (menuPosition == MENU_MOVE_XZ && btn == 2) endFileJob();
        else menuPosition++;
        if (menuPosition == 4){
          menuPosition = 1;
          setPause(0);
        }
      return;
    } 
    switch (menuPosition){
      case MENU_MOVE_XY: case MENU_MOVE_XZ:
        if (btn == 1) {
          motors.stopMotor(menuPosition - 1);
          menuPosition = 5 - menuPosition;
        }
        else menuPosition = MENU_TOOL_ON_OFF;
      break;
      case MENU_TOOL_ON_OFF: if (btn == 2) {tool(2);} else {tool(0);} break;
      case MENU_SET_POSITION_ZERO: 
        if (btn == 2) {motors.setMotorPosition(0, 0); motors.setMotorPosition(1, 0); RETURN_TO_MOVEMENT_MENU;}
        else motors.setMotorPosition(2, 0);
      break;
      case MENU_SD: 
          fileIndexOffset = 0;
          if (cardAvailable) SD.end();
          cardAvailable = SD.begin(4);
          if (cardAvailable){menuPosition = MENU_SD_FILE_1;} 
      break;
      case MENU_EXIT_OR_SETTINGS:
        if (btn == 2) menuPosition = MENU_SETTINGS;
        else menuPosition = MENU_MOVE_XY;
      break;
      case MENU_Z_PROBE:
        if (btn == 1) {
          RETURN_TO_MOVEMENT_MENU;
          printScreen(); 
          zProbe(true);
          menuPosition = MENU_Z_PROBE;
        }
        else if (cardAvailable) {
          RETURN_TO_MOVEMENT_MENU;
          externalInputMovement = 4;
        }
      break;
      case MENU_MOVE_XY_HOME: if (btn == 1) externalInputMovement = 5; else externalInputMovement = 6; RETURN_TO_MOVEMENT_MENU; break;
      case MENU_DISABLE_STEPPERS_ENDSTOP:
        setEndstop(endstopMask);
        PRINT_LCD;
      break;      
      case MENU_SAVE_RECALL_POSITION:
        if (btn == 2) loadMotorPosition();
        else saveMotorPosition();
        RETURN_TO_MOVEMENT_MENU;
      break;
      case MENU_SD_FILE_1: case MENU_SD_FILE_2: case MENU_SD_FILE_3: case MENU_SD_FILE_4:
        openFileIndex(menuPosition - MENU_SD_FILE_1);
      break;
      case MENU_SETTINGS_LOAD_DEFAULT:   
        EEPROMWriteint(0, 0);
        resetFunc();
      break;
      default: exportSettings(); menuPosition = MENU_EXIT_OR_SETTINGS; break;
    }
  }
  TEST_FREE_MEMORY;
}


uint8_t readBtn(){  //Return 1 if short pressed & released, Return 2 if long pressed PRIOR to release
  static unsigned long debounceTimer;
  static byte buttonPos;
  static byte _b;
  byte _c = digitalRead(CONTROL_CLICK_PIN);
  if (_b != _c) _b = _c;
  else if (!_c){   //Button depressed
    if (!buttonPos && millis() > debounceTimer + BUTTON_DEBOUNCE){buttonPos = 1; debounceTimer = millis();}
    else if (buttonPos == 1 && millis() > debounceTimer + BUTTON_LONGHOLD){buttonPos = 2; return 2;}
  }
  else if (buttonPos) {
    if (buttonPos == 1 && millis() > debounceTimer + 10) {buttonPos = 0; return 1;}
    buttonPos = 0;
  }
  return 0;
}
