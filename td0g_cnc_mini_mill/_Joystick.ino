/////////////////////////////////////////////////////////

//                         Joystick

/////////////////////////////////////////////////////////

#define MENU_SETTINGS_PROBE_GRID 20
#define MENU_SETTINGS_PROBE_PLUNGE 21
#define MENU_SETTINGS_PROBE_RETURN 22



void runJoystick(){
  static int8_t lastdir[2];

//Not moving motors
  if (menuPosition & 0b11111100) {  
    if (adcReading[1] == -2) adcReading[1]++;
    else if (adcReading[1] == 2) adcReading[1]--;
    if (adcReading[0] == -2) adcReading[0]++;
    else if (adcReading[0] == 2) adcReading[0]--;
    
//Is there X-direction?
    if (adcReading[0] != lastdir[0]){
      if ((menuPosition & 0b11111100) == 16) motors.maxSpeed(menuPosition-16,10 * adcReading[0] + motors.maxSpeed(menuPosition-16));
      else if (menuPosition == MENU_SETTINGS_PROBE_GRID) probeGridDist += adcReading[0];
      else if (menuPosition == MENU_SETTINGS_PROBE_PLUNGE) probePlunge = probePlunge + adcReading[0];
      else if (menuPosition == MENU_SETTINGS_PROBE_RETURN) probeReturn = probeReturn + adcReading[0];
      PRINT_LCD;
    }

//No X-direction, is there Y-direction?
    else if (adcReading[1] != lastdir[1]){
      menuPosition -= adcReading[1];
      if (adcReading[1] > 0) {
        if (menuPosition == 3) menuPosition = 11;  //Returning from top of menu
        else if (menuPosition == 11) menuPosition = 6;
        else if (menuPosition == 15) {
          importSettings();
          menuPosition = 10;
        }
      }
      else if (adcReading[1] < 0) {
        if (menuPosition == 12) menuPosition = 4; //lcd.clear();}
        else if (menuPosition == 16) {
          menuPosition = 12;
          fileIndexOffset -= 4;
          PRINT_LCD; //printDirectory();
        }
        else if (menuPosition == 28) menuPosition = 16;
      }
    }
  }

//Joystick is moving tool 
  else if (menuPosition & 0b11111110) { 
    for (byte i = 0; i < 2; i++){
      byte j = i * (menuPosition - 1); //Just choosing which motor to use, 0 1 or 2 (x y or z)
      if (!externalInputMovement){
        if (!adcReading[i] ||(motors.moving(j) != 0 && ((motors.targetPosition(j) < 0 && adcReading[i] > 0) || (motors.targetPosition(j) > 0 && adcReading[i] < 0)))) motors.accelerate(j, -4, 80);
        else if (!motors.moving(j)){
          if (adcReading[i] > 0) motors.moveMotor(j, 30);
          else motors.moveMotor(j, -30);
        }
        else if (adcReading[i] == 2 || adcReading[i] == -2) motors.accelerate(j, 4, 80);
      }
      else if (lastdir[i] != adcReading[i] && abs(adcReading[i]) == 2){
        if (menuPosition == 3 && i == 0){
          speedMultiplier += adcReading[i]*5;
          speedMultiplier = max(10, speedMultiplier);
          PRINT_LCD;
        }
        else motors.nudge(j, 8*adcReading[i],12);
      }
    }
  }
  lastdir[0] = adcReading[0];
  lastdir[1] = adcReading[1];  
}
