void importSettings(){
  byte _address;
  _address = 0;
  if (EEPROMReadint(0) == 0b1111111111111111 || !EEPROMReadint(0)) exportSettings();
  for (byte i = 0; i < 3; i++){
    stepRateMax[i] = EEPROMReadint(_address);
    axisSize[i] = EEPROMReadint(_address + 6);
    _address += 2;
  }
  EEPROM.get(12, probePlunge);
  probeGridDist = EEPROM.read(16);
}

void exportSettings(){
  for (byte i = 0; i < 3; i++){
    EEPROMWriteint(i * 2, stepRateMax[i]);
    EEPROMWriteint(i * 2 + 6, axisSize[i]);
  }
  EEPROM.put(12, probePlunge);
  EEPROM.update(16, probeGridDist);
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
