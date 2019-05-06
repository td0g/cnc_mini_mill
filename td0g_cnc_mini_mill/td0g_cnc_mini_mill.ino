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
  
  
VALID GCODE
 G0 / G1: MOVE
 G29: PROBE
 G30: PRINT POSITION
 G90: ABSOLUTE MOVE
 G91: RELATIVE MOVE
 G92: SET POSITION
 M3: SPINDLE ON
 M5: SPINDLE OFF
 M105: RETURN DUMMY TEMPERATURE
 M114: GET POSITION
 M119: GET ENDSTOP STATE
 */

//General Definitions
  #define VERSION "1.1"

  #define BAUD 57600
  #define MAX_BUF 40 // What is the longest message Arduino can store?
  #define BUTTON_DEBOUNCE 300
  #define BUTTON_LONGHOLD 600
  #define DEFAULT_PROBE_GRID_DIST 25
  #define DEFAULT_PROBE_PLUNGE 1.8

  #define DEFAULT_X_SIZE 26250  //175mm
  #define DEFAULT_Y_SIZE 18750  //150mm
  #define DEFAULT_Z_SIZE 13120  //17.5mm

  #define FEEDRATE_DEFAULT 60 //mm/min
  
  #define X_STEPRATE_MAX 300
  #define Y_STEPRATE_MAX 300
  #define Z_STEPRATE_MAX 300
  #define X_STEPRATE_MAX_JOYSTICK 2000
  #define Y_STEPRATE_MAX_JOYSTICK 1500
  #define Z_STEPRATE_MAX_JOYSTICK 2000

  #define X_STEPS_PER_MM 150  //2 mm/s
  #define Y_STEPS_PER_MM 125  //2.3 mm/s
  #define Z_STEPS_PER_MM 750  //0.4 mm/s

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


//General
  byte menuPosition = 2;
  #define PRINT_LCD menuPosition = menuPosition | 0b10000000

//Serial
  char serialBuffer[MAX_BUF];
  int sofar;
  byte left;
  const char axisLetter[3] = {'X', 'Y', 'Z'};

//Mechanics
  unsigned long dwellTimer;
  byte posABS = 1;
  byte inputType = 0;
  float probePlunge = DEFAULT_PROBE_PLUNGE;
  byte probeGridDist = DEFAULT_PROBE_GRID_DIST;

//ADC
  int8_t adcReading[] = {0, 0};
  byte endstopState;
  byte joystickPosition;

//Motor
  #include <AccelStepper.h>
  AccelStepper x(1, X_STEP_PIN, X_DIR_PIN);
  AccelStepper y(1, Y_STEP_PIN, Y_DIR_PIN);
  AccelStepper z(1, Z_STEP_PIN, Z_DIR_PIN);
  unsigned int stepRateMax[3] = {X_STEPRATE_MAX, Y_STEPRATE_MAX, Z_STEPRATE_MAX};
  int stepPerMM[3] = {X_STEPS_PER_MM, Y_STEPS_PER_MM, Z_STEPS_PER_MM};
  AccelStepper* xyzMotors[3] = {&x, &y, &z};
  byte pause = 0;
  int minmax[3][2] = {{-DEFAULT_X_SIZE, DEFAULT_X_SIZE},
                      {-DEFAULT_Y_SIZE, DEFAULT_Y_SIZE},
                      {-DEFAULT_Z_SIZE, DEFAULT_Z_SIZE}};
  unsigned int axisSize[3] = {DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE};

//SD Card
  #include <SPI.h>
  #include <SD.h>
  byte fileIndex[4];
  File root;
  File dataFile;
  byte readFromSD;
  byte cardAvailable;
  unsigned long fileSize;
  unsigned long fileProg;

//LCD
  #include <Wire.h>
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

void setup() {
//Serial
  Serial.begin(BAUD);

//SD
  cardAvailable = SD.begin(4);
  
//LCD    
  lcd.begin (20,4);  // initialize the lcd
  lcd.backlight();
  Wire.setClock(400000); // 1000000 or 400000
  
//ADC & UI
  ADCSRA =  bit (ADEN);   // turn ADC on
  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);
  pinMode(CONTROL_CLICK_PIN, INPUT_PULLUP);

//Tool
  pinMode(TOOL_PIN, OUTPUT);
  tool(0);

//EEPROM Settings
  importSettings();

//Other setup
  PRINT_LCD;
  Serial.print(F("TD0G v"));
  Serial.print(VERSION);
  Serial.println(F(" ['?']")); //Do we really need this?  Repetier Host seems to think so...
  while (millis() < 500){
    analogReadAll();
    runUI();
  }

//Motors
  pinMode(XY_EN_PIN, OUTPUT);
  pinMode(Z_EN_PIN, OUTPUT);
  for (byte i = 0; i < 3; i++) xyzMotors[i] -> setAcceleration(ACCELERATION);
  x.setPinsInverted(true, false, true);
  y.setPinsInverted(true, false, true);
  z.setPinsInverted(false, false, true); //first is dir
}

void loop(){
  analogReadAll();    //Get endstop state and joystick position, report using global variables
  runMotors();        //Using Accelstepper library, also watch and respond to endstop status
  runUI();            //Respond to button presses
  runJoystick();      //Respond to joystick movements
  getCommand();       //Respond to serial communications
  runLCD();           //Print position to LCD without blocking, one character at a time
  printScreen();      //Update LCD when requested      
  if (menuPosition < 4) printPositionscreen();  //Pass position to runLCD function
}
