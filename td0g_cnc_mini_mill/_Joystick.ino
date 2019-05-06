/////////////////////////////////////////////////////////

//                         Joystick

/////////////////////////////////////////////////////////

//Proposed 0000 = neutral, 0001 = up1, 0011 = up2, 0101 = dn1, 0111 = dn2

void runJoystick(){
  static int8_t lastdir[2];
  byte _menuPosition;
  _menuPosition = menuPosition >> 2;
  if (_menuPosition) {  //Not moving motors
    if (adcReading[1] == -2) adcReading[1] = -1;    //Y: No analog response - only direction
    else if (adcReading[1] == 2) adcReading[1] = 1; //Y: No analog response - only direction
    if (abs(adcReading[0]) == 1) adcReading[0] = 0;         //X: Decrease sensitivity

    //Is there X-direction?
    if (adcReading[0] != lastdir[0]){
      lastdir[0] = adcReading[0];
      lastdir[1] = adcReading[1];
      if (_menuPosition == 4){
        if (adcReading[0] == -2) stepRateMax[menuPosition - 16] -= 10;
        else if (adcReading[0] == 2) stepRateMax[menuPosition - 16] += 10;
      }
      else if (_menuPosition == 5){
        if (adcReading[0] == -2) axisSize[menuPosition - 20] -= 10;
        else if (adcReading[0] == 2) axisSize[menuPosition - 20] += 10;
      }
      else if (menuPosition == 24){
        if (adcReading[0] == -2) probeGridDist--;
        else if (adcReading[0] == 2) probeGridDist++;
      }
      else if (menuPosition == 25){
        if (adcReading[0] == -2) probePlunge -= 0.1;
        else if (adcReading[0] == 2) probePlunge += 0.1;
      }
      PRINT_LCD;
    }

    //No X-direction, is there Y-direction?
    if (adcReading[1] != lastdir[1]){
      lastdir[1] = adcReading[1];
      if (lastdir[1] > 0) {
        if (menuPosition == 4) menuPosition = 11;
        else if (menuPosition == 12) menuPosition = 6;
        else if (menuPosition == 16) {
          importSettings();
          menuPosition = 10;
        }
        else menuPosition--;
      }
      else if (lastdir[1] < 0) {
        if (menuPosition == 11) {menuPosition = 4; lcd.clear();}
        else if (menuPosition == 15) menuPosition = 12;
        else if (menuPosition == 27) menuPosition = 16;
        else menuPosition++;
      }
    }
  }
  else if (!inputType){ //Joystick is moving tool 
    for (byte i = 0; i < 2; i++){
      if (lastdir[i] != adcReading[i]){
        byte j = i * (menuPosition - 1); //Just choosing which motor to use, 0 1 or 2 (x y or z)
        switch (adcReading[i]){
          case -2: xyzMotors[j] -> setMaxSpeed(stepRateMax[j] * 2); xyzMotors[j]->moveTo(minmax[j][0]); break;
          case -1: if (!lastdir[i]) xyzMotors[j] -> setMaxSpeed(stepRateMax[j] / 2); xyzMotors[j]->moveTo(minmax[j][0]); break;
          case 0: xyzMotors[j]->stop(); break;
          case 1: if (!lastdir[i]) xyzMotors[j] -> setMaxSpeed(stepRateMax[j] / 2); xyzMotors[j]->moveTo(minmax[j][1]); break;
          case 2: xyzMotors[j] -> setMaxSpeed(stepRateMax[j] * 2); xyzMotors[j]->moveTo(minmax[j][1]); break;
        }
        lastdir[i] = adcReading[i];
      }    
    }
  }
}
