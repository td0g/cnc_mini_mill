byte _charRemaining;
char _charArray[8];
uint32_t LCDtimer;

boolean runLCD(){
  static byte charPos;
  if (!_charRemaining) return true;
  else if (millis() > LCDtimer){
    lcd.print(_charArray[charPos]);
    charPos++;
    if (charPos == _charRemaining) {
      _charRemaining = 0;
      charPos = 0;
    }
    LCDtimer = millis() + 25;
  }
  return false;
}

void printFltNB(byte column, byte row, float oFloat, byte decimalPlaces, byte space){
  unsigned long t;
  byte j;
  while (!runLCD()) runMotors();
  appendCharArray(' ');
  runMotors();
  if (oFloat < 0) appendCharArray('-');
  runMotors();
  t = fabs(oFloat);
  runMotors();
  if (!t)appendCharArray('0');
  else {
    j = 0;
    while (t >= pow(10, j)) {
      j++;
      runMotors();
    }
    while (j){
      runMotors();
      j--;
      appendCharArray((t / uint32_t(pow(10, j))) % 10 + '0');
    }
  }
  appendCharArray('.');
  while (j < decimalPlaces){
    runMotors();
    j++;
    appendCharArray(int((abs(oFloat) * pow(10, j))) % 10 + '0');
  }
  if (_charRemaining > space){
    j = _charRemaining - space;
    for (byte k = 0; k < space; k++){
      runMotors();
      _charArray[k] = _charArray[k+j];
    }
    _charRemaining = space;
  }
  lcd.setCursor(column - _charRemaining + 1, row);
  LCDtimer += 50;
}

void appendCharArray(char c){
  _charArray[_charRemaining] = c;
  _charRemaining++;
}
