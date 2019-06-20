void printDirectory() {
  root = SD.open("/");
  byte i = 0;
  byte j = 0;
  dataFile =  root.openNextFile();
  while (i < 4 && dataFile) {
    j++;
    if (!dataFile.isDirectory()) {
      fileIndex[i] = j;
      lcd.setCursor(1, i);
      lcd.print(dataFile.name());
      i++;
    }
    dataFile.close();
    dataFile = root.openNextFile();
  }
  root.close();
  while (i < 4){
    fileIndex[i]=0;
    i++;
  }
}


void openFileIndex(byte _index){
  root = SD.open("/");
  unsigned long i = 0;
  byte j = 0;
  while (j < _index) {
    dataFile =  root.openNextFile();
    j++;
    if (!dataFile) break;
    if (j < _index) dataFile.close();
  }
  fileSize = dataFile.size();
  root.close();
  menuPosition = 2;
  readFromSD = 1;
  if (_index == fileResumeIndex && fileProg){
    i = 0;
    j = 0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Begin<  >Resume"));
    lcd.setCursor(0, 1);
    lcd.print(fileProg);
    lcd.print(F(" / "));
    lcd.print(fileSize);
    while (adcReading[0] == 0) analogReadAll();
    if (adcReading[0] > 0){
      while (i < fileProg){
        dataFile.read();
        i++;
        if (!j){
          lcd.setCursor(0,2);
          lcd.print(i);
        }
        j++;
      }
    }
    else fileProg = 0;
  }
  else fileProg = 0;
  fileResumeIndex = _index;
}

void endFileJob(){
  fileProgThisLine = 0;
  fileProgNextLine = 0;
  readFromSD = 0;
  dataFile.close();
  menuPosition = 2;
  setPause(0);
  //x.stop();
  //y.stop();
  //z.stop();
  motors.stopOneMotor(0);
  motors.stopOneMotor(1);
  motors.stopOneMotor(2);
}
