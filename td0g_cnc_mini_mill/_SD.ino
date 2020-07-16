void printDirectory() {
  root = SD.open("/");
  byte i = fileIndexOffset;
  while (i != 4) {
    dataFile =  root.openNextFile();
    if (!dataFile) i = 4;
    else if (!dataFile.isDirectory()) {
      if (!(i & 0b11111100)){
        lcd.setCursor(1, i);
        lcd.print(dataFile.name());
      }
      i++;
    }
    dataFile.close();
  }
  TEST_FREE_MEMORY;
  root.close();
}


void openFileIndex(byte _index){
  Serial.println(_index);
  root = SD.open("/");
  byte i = fileIndexOffset - 1;
  while (i != _index) {
    if (dataFile) dataFile.close();
    dataFile =  root.openNextFile(); //Skipping directories like the print function
    if (!dataFile.isDirectory()) i++;
  }
  root.close();
  externalInputMovement = 2;
  menuPosition = 1;
  unsigned long j = 0;
  if (_index == fileResumeIndex && fileProg && dataFile.size() == EEPROMReadlong(EEPROM_FILE_POSITION_LOCATION + 6)){
    //i = 0;
    i = 0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Begin<  >Resume"));
    lcd.setCursor(0, 1);
    lcd.print(fileProg);
    //lcd.print(F(" / "));
    //lcd.print(fileSize);
    while (adcReading[0] == 0) analogReadAll();
    if (adcReading[0] > 0){
      while (j < fileProg){
        char _c = dataFile.read();
        if (_c == 10 || _c == 13) j++;
        if (!i){
          lcd.setCursor(0,2);
          lcd.print(j);
        }
        i++;
      }
    }
    else fileProg = 0;
    //while (adcReading[0] != 0)analogReadAll();
  }
  else fileProg = 0;
  fileResumeIndex = _index;
  saveFilePosition();
  TEST_FREE_MEMORY;
}

void endFileJob(){
  if (externalInputMovement == 2){
    fileProg -= motors.queues();
    saveFilePosition();
    dataFile.close();
  }
  motors.stopMotor(0);
  motors.stopMotor(1);
  motors.stopMotor(2);
  tool(0);
  setPause(0);
  externalInputMovement = 0;
  sofar = 0;
  RETURN_TO_MOVEMENT_MENU;
}
