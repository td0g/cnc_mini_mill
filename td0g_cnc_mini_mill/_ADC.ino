void analogReadAll(){
  static byte adcNumber;
  static const byte adcPins[3] = {0b01100000 | (CONTROL_X_PIN_ANALOG & 31), 0b01100000 | (CONTROL_Y_PIN_ANALOG & 31), 0b01100000 | (ENDSTOP_PIN_ANALOG & 31)};//CONTROL_X_PIN_ANALOG, CONTROL_Y_PIN_ANALOG, ENDSTOP_PIN_ANALOG};
  static byte lastEndstopState;
  static byte lastadc;
  const byte adcValues[9] = {10, 52, 75, 96, 111, 122, 133, 150, 255}; //last 255 doesn't mean anything               
  const byte endstopValues[8] = {0b00000111, 0b00000011, 0b00000110, 0b00000010, 0b00000101, 0b00000100, 0b00000001, 0};
  const byte joystickValues[5] = {25, 100, 150, 225, 255};
  if (ADCSRA & 0b01000000) return;//bit_is_clear(ADCSRA, ADSC)){
  byte adc = ADCH;
  if (adcNumber == 2){  //interpret endstop state
    if (abs(adc - lastadc) < 3){
      byte thisEndstopState = 0;
      while (adc > adcValues[thisEndstopState]) thisEndstopState++;
      if (thisEndstopState == 8)  globalEndstopState = globalEndstopState | 0b10000000;
      else {
        thisEndstopState = endstopValues[thisEndstopState];
        globalEndstopState = globalEndstopState & 0b01111111; //probeHit = 0
        if (thisEndstopState == lastEndstopState && globalEndstopState != lastEndstopState) {
          for (byte i = 0; i < 3; i++){
            if ((((thisEndstopState & endstopMask) >> i) & 1) && !(globalEndstopState & (0b000001001 << i)) && motors.distanceToGo(i) != 0){
              if (motors.distanceToGo(i) > 0) {
                lastEndstopState |= 1 << (i+3); //bitSet(lastEndstopState, i+3);
                lastEndstopState &= ~(1 << i); //bitClear(lastEndstopState, i);
                motors.setMotorMaxPositionHere(i);
              }
              else if (motors.distanceToGo(i) < 0) motors.setMotorMinPositionHere(i);
              if (!externalInputMovement) motors.stopMotor(i);
            }
          }
          globalEndstopState = lastEndstopState; //endstopState only updated if changes are noticed
        }
        lastEndstopState = thisEndstopState;
      }
      globalEndstopState &= endstopMask;
      adcNumber = 0;
    }
    lastadc = adc;
  }
  else {
    adcReading[adcNumber] = -2;
    while (adc > joystickValues[adcReading[adcNumber]+2]) adcReading[adcNumber]++;
    adcNumber++;
  }
  ADMUX = adcPins[adcNumber];//0b01100000 | (adcPin & 31); 
  ADCSRB = 1; //FOR ATMEGA2560: ((adcPin & 0b00100000) >> 2) + 1;
  ADCSRA = 0b11010111; //Enable ADC
  //}
}
