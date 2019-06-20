void importSettings(){
  byte _address;
  _address = 0;
  if (EEPROMReadint(0) == 0b1111111111111111 || !EEPROMReadint(0)) exportSettings();
  for (byte i = 0; i < 3; i++){
    stepRateMax[i] = EEPROMReadint(_address);
    _address += 2;
  }
  probePlunge = EEPROM.read(12);
  probePlunge /= 10;
  probeGridDist = EEPROM.read(13);
}

void exportSettings(){
  for (byte i = 0; i < 3; i++){
    EEPROMWriteint(i * 2, stepRateMax[i]);
  }
  byte _b;
  _b = probePlunge * 10;
  EEPROM.update(12, _b);
  EEPROM.update(13, probeGridDist);
}

unsigned int EEPROMReadint(unsigned int _i){
  unsigned int _t;
  _t = EEPROM.read(_i);
  _t *= 256;
  _t += EEPROM.read(_i + 1);
  return (_t);
}
/*
void EEPROMWriteint(unsigned int _i, unsigned long _v, byte _s){
  for (byte i = 0; i < _s; i++){
    byte _b = ((_v >> (8 * i)) & 0xFF);
    EEPROM.update(_i + i, _b);
  }
}
*/
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
