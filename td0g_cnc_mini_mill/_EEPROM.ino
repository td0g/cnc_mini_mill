void importSettings(){
  byte _address;
  _address = EEPROM_SETTINGS_LOCATION;
  if (EEPROMReadint(EEPROM_SETTINGS_LOCATION) == 0b1111111111111111 || !EEPROMReadint(EEPROM_SETTINGS_LOCATION)) exportSettings();
  for (byte i = 0; i < 3; i++){
    motors.maxSpeed(i, EEPROMReadint(_address));
    _address += 2;
  }
  probePlunge = EEPROM.read(12 + EEPROM_SETTINGS_LOCATION);
  probeReturn = EEPROM.read(14 + EEPROM_SETTINGS_LOCATION);
  probeGridDist = EEPROM.read(13 + EEPROM_SETTINGS_LOCATION);
}

void exportSettings(){
  //byte _b;
  for (byte _b = 0; _b < 3; _b++)EEPROMWriteint(_b * 2, motors.maxSpeed(_b));
  EEPROM.update(12, probePlunge);
  EEPROM.update(13, probeGridDist);
  EEPROM.update(14, probeReturn);
}

unsigned int EEPROMReadint(unsigned int _i){
  unsigned int _t;
  _t = EEPROM.read(_i);
  _t *= 256;
  _t += EEPROM.read(_i + 1);
  return (_t);
}

void EEPROMWriteint(unsigned int _i, unsigned int _d){
  byte t;
  t = _d;
  EEPROM.update(_i+1, t);
  t = _d >> 8;
  EEPROM.update(_i, t);
}

void EEPROMWritelong(unsigned int address, long value) {
  for (byte i = 0; i < 4; i++){
    byte _b = ((value >> (8 * i)) & 0xFF);
    EEPROM.update(address + i, _b);
  }
}

long EEPROMReadlong(int address){
  unsigned long _d;
  address += 3;
  for (byte i = 0; i < 4; i++){
    _d = _d << 8;
    _d += EEPROM.read(address - i);
  }
return _d;
}

void saveMotorPosition(){
  EEPROMWriteint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES, motors.currentPosition(0) + 32000);
  EEPROMWriteint(EEPROM_POSITION_LOCATION+2 + EEPROM_POSITION_SIZE_BYTES, motors.currentPosition(1) + 32000);
  EEPROMWriteint(EEPROM_POSITION_LOCATION+4 + EEPROM_POSITION_SIZE_BYTES, motors.currentPosition(2) + 32000);
}

void clearMotorPosition(){
  EEPROMWriteint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES, 32000);
  EEPROMWriteint(EEPROM_POSITION_LOCATION+2 + EEPROM_POSITION_SIZE_BYTES, 32000);
  EEPROMWriteint(EEPROM_POSITION_LOCATION+4 + EEPROM_POSITION_SIZE_BYTES, 32000);
}

void loadMotorPosition(){
  motors.setMotorPosition(0, (int)EEPROMReadint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES) - 32000);
  motors.setMotorPosition(1, (int)EEPROMReadint(EEPROM_POSITION_LOCATION+2 + EEPROM_POSITION_SIZE_BYTES) - 32000);
  motors.setMotorPosition(2, (int)EEPROMReadint(EEPROM_POSITION_LOCATION+4 + EEPROM_POSITION_SIZE_BYTES) - 32000);
}

void saveFilePosition(){
  EEPROMWriteint(EEPROM_FILE_POSITION_LOCATION, fileResumeIndex);
  EEPROMWritelong(EEPROM_FILE_POSITION_LOCATION + 2, fileProg);
  //EEPROMWritelong(EEPROM_FILE_POSITION_LOCATION + 6, fileSize);
  EEPROMWritelong(EEPROM_FILE_POSITION_LOCATION + 6, dataFile.size());
}

void loadFilePosition(){
  fileResumeIndex = EEPROMReadint(EEPROM_FILE_POSITION_LOCATION);
  fileProg = EEPROMReadlong(EEPROM_FILE_POSITION_LOCATION + 2);
}
