void printPositionscreen(){
  static byte rowXYZ;
  static unsigned long lastFileProg;
  static unsigned long timer;
  if (menuPosition < 4 && millis() > timer){
    timer += LCD_REFRESH_INTERVAL;
    rowXYZ = (rowXYZ + 1) % 4;
    if (rowXYZ == 3){
        if (readFromSD && (lastFileProg != fileProg)){
          unsigned long _t;
          _t = fileProg * 100;
          _t /= fileSize;
          lcd.setCursor(15, 0);
          lcd.print(_t);
        }
    }
    else {
      byte _c;
      _c = 3;
      float _p = getMM(rowXYZ);
      if (_p < 0) _c--;
      _p = fabs(_p);
      if (_p < 100) {
        _c++;
        if (_p < 10) {
          _c++;
        }
      }
      lcd.setCursor(_c, rowXYZ + 1);
      lcd.print(" ");
      lcd.print(getMM(rowXYZ),3);
    }
  }
}
