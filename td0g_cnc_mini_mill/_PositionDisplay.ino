void printPositionscreen(){
  static byte rowXYZ;
  static unsigned long timer;
  if (menuPosition & 0b11111100) return;
  if (millis() > timer){
    timer = millis() + LCD_REFRESH_INTERVAL;
    if (rowXYZ == 3){
        rowXYZ = 0;
        if (externalInputMovement == 2){
          unsigned long _t;
          _t = dataFile.position();
          _t *= 100;
          _t /= dataFile.size();
          //_t = fileProg * 100;
          //_t /= fileSize;
          lcd.setCursor(15, 0);
          lcd.print(_t);
        }
        lcd.setCursor(16, 1);
        TEST_FREE_MEMORY;
        lcd.print(minMemory);
        //motors.printDebug();
        //RESET_FREE_MEMORY;
    }
    else {
      byte _c;
      _c = 3;
      float _p = getMM(rowXYZ);
      rowXYZ++;
      if (_p < 0) _c--;
      if (_p < 100 && _p > -100) {
        _c++;
        if (_p < 10 && _p > -10) _c++;
      }
      lcd.setCursor(_c, rowXYZ);
      lcd.print(F(" "));
      lcd.print(_p, 3);
    }
  }
}
