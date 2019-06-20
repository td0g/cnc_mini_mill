void analogReadAll(){
  static byte adcNumber;
  static const byte adcPins[3] = {CONTROL_X_PIN_ANALOG, CONTROL_Y_PIN_ANALOG, ENDSTOP_PIN_ANALOG};
  static byte newEndstopState;
  static byte lastadc;
  const byte adcValues[9] = {10, 52, 75, 96, 111, 122, 133, 150, 255};               
  const byte endstopValues[8] = {0b00000111, 0b00000011, 0b00000110, 0b00000010, 0b00000101, 0b00000100, 0b00000001, 0};
  const byte joystickValues[5] = {25, 100, 150, 225, 255};
  const byte joystickShift[2] = {0b00000111, 0b00111000};
  
  if (bit_is_clear(ADCSRA, ADSC)){
    byte adc = ADC >> 2;
    if (adcNumber == 2){  //interpret endstop state
      if (abs(adc - lastadc) < 3){
        byte i = 0;
        while (adc > adcValues[i]) i++;
        if (i == 8)  endstopState = endstopState | 0b10000000;//probeHit = 1
        else {
          endstopState = endstopState & 0b01111111; //probeHit = 0
          if (endstopValues[i] == newEndstopState && endstopState != newEndstopState) { //1.4
            for (byte i = 0; i < 3; i++){
              if (motors.distanceToGo(i) == 0) bitClear(newEndstopState, i);
              else if (motors.distanceToGo(i) > 0 && ((newEndstopState >> i) & 1)) {
                bitSet(newEndstopState, i+3);
                bitClear(newEndstopState, i);
              }
            }
            endstopState = newEndstopState;
            endstopHit();
          }
          newEndstopState = endstopValues[i];
        }
        endstopState &= endstopMask;
        adcNumber = 0;
      }
      lastadc = adc;
    }
    else {
      adcReading[adcNumber] = 0;
      while (adc > joystickValues[adcReading[adcNumber]]) adcReading[adcNumber]++;
      adcReading[adcNumber] -= 2;
      adcNumber++;
    }
    byte adcPin;
    adcPin = adcPins[adcNumber];
    ADMUX =   bit (REFS0) | (adcPin & 31);  // AVcc  
    ADCSRB = ((adcPin & 0b00100000) >> 2) + 1;
    bitSet(ADCSRA, ADSC);
  }
}
