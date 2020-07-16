

/*
  Written by Tyler Gerritsen
  vtgerritsen@gmail.com
  
  Thanks to https://www.marginallyclever.com/2013/08/how-to-build-an-2-axis-arduino-cnc-gcode-interpreter/ 
  Serial buffer size increase:  Thanks to https://internetofhomethings.com/homethings/?p=927
  https://github.com/fmalpartida/New-LiquidCrystal - Required I2C LCD library
  
  
Version History

0.1 - 0.3
  2017-10-20
  Really poorly documented, I don't remember...
  
0.4
  2018-12-30
  Working with the new ATMega328P Board

0.5
  2018-12-30
  Implementing AccelStepper

0.6
  2019-03-31
  Introduced SD card support (NOT STABLE)
  Fixed Z-Probing
  Lots of improvements to motion
  Removed use of Multistepper library

0.7
  2019-04-19
  SD Card Supported
  Improved endstop response function

1.0
  2019-05-05
  Full feature set!  Mill is ready for standalone functionality with SD card.
  Added standalone grid probe function (longhold on menu selection)
  Can remove and re-insert SD
  Fixed SD memory leak

1.1
  2019-05-05
  Added EEPROM settings
  Added percent completion for SD jobs

1.2
  2019-05-08
  Reduced overhead of calculating SD job percentage done
  Reduced overhead of position printing function
  Reduced size of EEPROM settings
  Fixed SD Bug
  Fixed gcode bug
  General code size reduction

1.3
  2019-05-26
  Added 'Resume Job' option for SD jobs
  Ignores second line ending character (CrLf)

1.4
  2019-06-14
  Removed build dimension limits - only relies on limit switches now
  M120, M121 gcode support
  Z-Probing origin now sets Z=0

2.0
  2019-06-16
  Replaced Accelstepper with DogStep
  Shifting during job
  Improvement to button read algorithm

2.1
  2019-07-23
  Fixed SD resume/restart control bug
  Single Probe now sets Z=0
  Can end probe cycle by pressing button
  When job is paused, position is saved to EEPROM and machine can be shut down
  Also can manually save/clear position in menu
  M17/M18 Support for enable/disable steppers
  Also can manually enable/disable steppers in menu

2.2
  2019-08-19
  Watchdog Timer -> removed as it wasnt doing any good
  Fixed Settings Bug
  File progress can be saved to EEPROM

2.3
  2019-08-29
  Feedrate Multiplier
  Accepts lowercase characters
  Multiple motor position save slots
  Removed all zProbe parameters - uses global variables ALWAYS
  New zProbe raise parameter (EEPROM)
  Fixed probe grid bugs
  Countdown probe grid probes left
  Fixed display errant character bug  

2.4
  2020-01-17
  Checks for F is 0 or negative
  Improvements to endstop algorithm
  Needs long-hold to turn on tool (short-press to turn off)
  Resume file function actually checks if it is the same file
  Simplified lineAbs function (doesn't need so much optimization anymore)
  Updates to d0gStep library
  Endstop and free memory displayed
  Option to disable endstops in menu
  Fixed Grid Probe doesn't end on 0
  Removed multiple position save slots - only one
  Removed automatic position save when pausing
  Z-probe reports 3 decimal places

2.5
  2020-03-02
  Update to d0gStep library
    Supports Multi-Stepping
    Supports movement queuing
  Fixed gcode parsing bug (_xyzf[4])

2.6
  2020-03-06
  Dynamic queue size
  Minor step algorithm enhancements
  Significantly reduced probe grid function
  More general optimization - Code size greatly reduced in preparation for d0gStep improvement
  Fixed file position save algorithm (broken when queueing was introduced)
  4-file limit on SD card dropped

2.7
  2020-06-20
  Fixed occasional freezing when pausing bug (new interruptSoon() function)
  Fixed max speed edit bug
  Fixed freezing on probing large grids bug
  Fixed unable to decelerate from 2849 step/s bug
  Manual jog max speed now 50% higher than standard max speed
  
  
VALID GCODE
 G0 / G1: MOVE
 G29: PROBE
 G30: PRINT POSITION
 G90: ABSOLUTE MOVE
 G91: RELATIVE MOVE
 G92: SET POSITION
 M3: SPINDLE ON
 M5: SPINDLE OFF
 M17: ENABLE STEPPERS
 M18: DISABLE STEPPERS
 M105: RETURN DUMMY TEMPERATURE
 M114: GET POSITION
 M119: GET ENDSTOP STATE
 M120: ENABLE ENDSTOPS
 M121: DISABLE ENDSTOPS
 */

//General Definitions
  #define VERSION "2.7"
  
  //#define MOTOR_DEBUG
  //#define NEW_JOYSTICK_SETUP

  #define BAUD 57600
  #define MAX_BUF 40 // What is the longest message Arduino can store?
  #define BUTTON_DEBOUNCE 300
  #define BUTTON_LONGHOLD 600
  #define DEFAULT_PROBE_GRID_DIST 5
  #define DEFAULT_PROBE_PLUNGE 1.8
  #define DEFAULT_PROBE_RETURN 0.5
  #define LCD_REFRESH_INTERVAL 60 //millis
  #define PROBE_DITHER 0.1

  #define FEEDRATE_DEFAULT_G0 120 //mm/min
  #define FEEDRATE_DEFAULT_G1 30 //mm/min
  
  #define X_STEPRATE_MAX 1150
  #define Y_STEPRATE_MAX 2500
  #define Z_STEPRATE_MAX 500

  #define X_STEPS_PER_MM 150  //2 mm/s
  #define Y_STEPS_PER_MM 750  //2.3 mm/s
  #define Z_STEPS_PER_MM 1500  //0.4 mm/s

  #define ACCELERATION 2000

//Pin Definitions
  #define TOOL_PIN 10
  #define CONTROL_CLICK_PIN 15

  #define CONTROL_X_PIN_ANALOG 2 //Analog number MUX, not pin number (page 282, 283 in ATMega2560 DS)
  #define CONTROL_Y_PIN_ANALOG 3 //Same
  #define ENDSTOP_PIN_ANALOG 0   //Same

  #define X_STEP_PIN 8
  #define X_DIR_PIN 9
  #define XY_EN_PIN 5
  #define Y_STEP_PIN 6
  #define Y_DIR_PIN 7
  #define Z_STEP_PIN 3
  #define Z_DIR_PIN 4
  #define Z_EN_PIN 2

  #define ENABLE_STEPPERS digitalWrite(XY_EN_PIN, 0); digitalWrite(Z_EN_PIN, 0);
  #define DISABLE_STEPPERS digitalWrite(XY_EN_PIN, 1); digitalWrite(Z_EN_PIN, 1); 
  #define IS_STEPPER_DISABLED (digitalRead(XY_EN_PIN))
  #define IS_MOTOR_POSITION_SAVED (EEPROMReadint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES) != 32000 || EEPROMReadint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES + 2) != 32000 || EEPROMReadint(EEPROM_POSITION_LOCATION + EEPROM_POSITION_SIZE_BYTES + 4) != 32000)
  #define RESET ////WATCHDOG_ON; while (1){};
  #define RETURN_TO_MOVEMENT_MENU menuPosition = 2////WATCHDOG_ON; menuPosition = 2;
  #define TEST_FREE_MEMORY minMemory = min(minMemory, freeMemory())
  #define RESET_FREE_MEMORY minMemory = freeMemory()
  #define FLOAT_NAN 0xFFFFFFFF

//General
  byte menuPosition = 2;
  #define PRINT_LCD menuPosition = menuPosition | 0b10000000
  #include <avr/wdt.h> /* Header for watchdog timers in AVR */

//Serial
  char serialBuffer[MAX_BUF+1];
  int sofar;
  const char axisLetter[3] = {'X', 'Y', 'Z'};

//Mechanics
  unsigned long dwellTimer;
  byte externalInputMovement = 0;
  byte probePlunge = DEFAULT_PROBE_PLUNGE * 10;
  byte probeReturn = DEFAULT_PROBE_RETURN * 10;
  byte probeGridDist = DEFAULT_PROBE_GRID_DIST;
  byte endstopMaskGlobal = 0b10101010;  //[Probe][][Zmax][Ymax][Xmax][Zmin][Ymin][Xmin]

//ADC
  int8_t adcReading[] = {0, 0};
  byte globalEndstopState;
  byte endstopMask = 0b10000000; //OFF default

//Motor
  #include "d0gStep.h"
  #define STEPPER_QUEUE_SIZE 3
  d0gStep motors(3, DISABLE_8_SECONDS, XY_EN_PIN, STEPPER_QUEUE_SIZE);
  const int stepPerMM[3] = {X_STEPS_PER_MM, Y_STEPS_PER_MM, Z_STEPS_PER_MM};
  const byte dither[2] = {X_STEPS_PER_MM * PROBE_DITHER, Y_STEPS_PER_MM * PROBE_DITHER};
  byte pause = 0;
  byte speedMultiplier = 100;

//SD Card
  #include <SPI.h>
  #include <SD.h>
  //byte fileIndex[4];
  File root;
  File dataFile;
  byte cardAvailable;
  int8_t fileIndexOffset;
  //unsigned long fileSize;
  unsigned long fileProg;
  byte fileResumeIndex;

//LCD
  //#include <Wire.h>
  #include <LCD.h>
  #include <LiquidCrystal_I2C.h>  // F Malpartida's NewLiquidCrystal library
  //Definitions for the LCD I2C module
    #define LCD_I2C_ADDR    0x27  // Define I2C Address for controller
    #define LCD_BACKLIGHT_PIN  3
    #define LCD_EN_PIN  2
    #define LCD_RW_PIN  1
    #define LCD_RS_PIN  0
    #define LCD_D4_PIN  4
    #define LCD_D5_PIN  5
    #define LCD_D6_PIN  6
    #define LCD_D7_PIN  7
    #define  LED_OFF  0
    #define  LED_ON  1
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,LCD_EN_PIN,LCD_RW_PIN,LCD_RS_PIN,LCD_D4_PIN,LCD_D5_PIN,LCD_D6_PIN,LCD_D7_PIN, LCD_BACKLIGHT_PIN, POSITIVE);

//EEPROM
  #include <EEPROM.h>
  #define EEPROM_SETTINGS_LOCATION 0
  #define EEPROM_POSITION_LOCATION 20
  #define EEPROM_POSITION_SIZE_BYTES 6
  #define EEPROM_FILE_POSITION_LOCATION 48

//DIAGNOSTICS
  #include <MemoryFree.h>
  int minMemory = 9999;

void setup() {
//Serial
  Serial.begin(BAUD);

//SD
  cardAvailable = SD.begin(4);
  
//LCD    
  lcd.begin (20,4);  // initialize the lcd
  
//ADC & UI
  ADCSRA =  bit (ADEN);   // turn ADC on
  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);
  pinMode(CONTROL_CLICK_PIN, INPUT_PULLUP);

//Tool
  pinMode(TOOL_PIN, OUTPUT);
  tool(0);


//Other setup
  PRINT_LCD;
  Serial.print(F("TD0G v"));
  Serial.print(F(VERSION));
  Serial.println(F(" ['?']")); //Do we really need this?  Repetier Host seems to think so...
  while (millis() < 500){
    analogReadAll();
    runUI();
  }

//Motors
  motors.attachMotor(X_STEP_PIN, X_DIR_PIN);
  motors.attachMotor(Y_STEP_PIN, Y_DIR_PIN);
  motors.attachMotor(Z_STEP_PIN, Z_DIR_PIN);
  pinMode(Z_EN_PIN, OUTPUT);

//EEPROM Settings (MUST come after attachMotor functions
  importSettings();
  
//Misc
  loadFilePosition();
}

void loop(){
  while (1) loopAll();
}

void loopAll(){
  #ifdef MOTOR_DEBUG
    static unsigned long _t;
    if (millis() > _t){
      _t += 5000;
      motors.printDebug();
      //wdt_reset();
    }
  #endif
  analogReadAll();    //Get endstop state and joystick position, report using global variables 
  runUI();            //Respond to button presses 
  runJoystick();      //Respond to joystick movements 
  getCommand();       //Respond to serial communications 
  printScreen();      //Update LCD when requested     
  printPositionscreen();  //Pass position to runLCD function  
  runMacros();        //Macro functions
}
