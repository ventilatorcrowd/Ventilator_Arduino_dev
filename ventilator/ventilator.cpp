#include <Arduino.h>

/* Author      : Rob Collins
   Version     : 15
   Description : Simple ventilator controller providing a simple human interface for Respiratory Rate, I.E. Ratio and Tidal volume
                 Ventilator control cycle runs from a timer interrupt .. enabling control parameters to be changed whilst the controler
                 continues to provide control signals
   Changes     : V15 : Introduces intermediate variables and a 'semaphor' between the parameter change code and the main control loop
                       This is to enable changed parameters to only take effect at the end of a breathing cycle.
                       Also avoid the problem of a race condition in the case where parameter variables are partially update when a control-loop interrupt occures
                       This version uses a 'semaphor' to signal to the main control loop that parameters have been updated. This semaphor is only set whilst the
                       interrupt timers are halted

               : V16 : This version has outpur pins re-assigned to match the circuit layout defined using circutio.io
               : V17 : Added servo control to simulate output to an actuator .. needed to use ServoTimer2.h as there is a conflict between TimerOne.h and Servo.h
               : V18 : Throw away version
               : V19 : Added a 'calibration' mode - when the mode switch is held down for an extended period - enters a calibration mode .. must be removed from deployed versions!
               : V20 : Includes pressure measurement from MPX5010DP
               : V21 : Throw away
               : V22 : Changed to PWM output
               : V23 : Changed the max tidal volume to 700 ml (single change of the constant defining this value)
               : V24 : Added a spontaneous breathing mode - triggers a breath cycle when transucer pressure drops below PEEP threshold
   Circuit     : https://www.circuito.io/app?components=512,9590,9591,11021,217614,417987
               : Also add a 10k pull-up resistor between Rotary centre switch (Middle pin, 'SELECT_BUTTON' ) and +5V .. this device has no built in pull-up for centre switch
   Status      : Untested
*/



#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>                        // Interrupt library to keep the control cycle regular .. https://playground.arduino.cc/Code/Timer1/

const bool PRODUCTION_CODE = false;        // This is non-production code - not tested and includes calibration routines

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4); // Address may sometimes be 0x3f

// Define shape of driver waveform
const int INHALE_DRIVE[]={0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,217,235,252,270,287,305,322,340,357,375,392,410,427,445,462,480,497,515,532,550,567,585,602,620,637,655,672,690,707,725,742,760,777,795,812,830,847,865,882,900,906,912,918,924,930,936,943,949,955,961,967,973,979,986,992,998,1004,1010,1016,1023};
const int DRIVE_TABLE_SIZE = 100;            // length of the INHALE_DRIVE table
const int DRIVE_VAL_MIN = 0;                 // minimum value for the output drive (position during exhale)

// define control constants for the ventilator
const int INSP_PRESS_MAX = 20;           // Inspiratory pressure
const int INSP_PRESS_MIN = 5;
const int INSP_PRESS_DEFAULT = 15;
const int INSP_PRESS_STEP = 1;

const int RESP_RATE_MAX = 30;             // Respratory rate
const int RESP_RATE_MIN = 5;
const int RESP_RATE_DEFAULT = 20;
const int RESP_RATE_STEP = 1;

const int TIDAL_MAX = 700;                   // TIDAL
const int TIDAL_MIN = 200;
const int TIDAL_DEFAULT = 250;
const int TIDAL_STEP = 10;

const float I_E_RATIO_MAX = 3.0;           // Inspiratory - expiratory ratio
const float I_E_RATIO_MIN = 0.2;
const float I_E_RATIO_DEFAULT = 1;
const float I_E_RATIO_STEP = 0.2;

const int INHALE_STATE = 0;     // vent is driving inhalation
const int EXHALE_STATE = 1;      // vent is driving exhlation

const int RAW_ACTUATOR_MIN = 0;  // minimum, unscalled value direct to actuatory
const int RAW_ACTUATOR_MAX = 1023; // max
const int RAW_ACTUATOR_STEP = 20;

const long int ENTER_CALIBRATION = 5000;  // Length of time in milli-seconds that the select button has to be pushed to enter calibration mode

// Pressure reading smoothing
const int PRESS_READ_SMOOTHING = 50 ;     // The number of pressure readings that get averaged to get a smooth result
int pressReadings[ PRESS_READ_SMOOTHING ]; // Actual data from the pressure sensor
int pressReadIndex = 0;                   // Index of the current pressure reading
int pressTotal = 0;                       // Running total for the pressure
int pressAverage = 0;                     // Average reading for the pressure

// Pressure sensor convert from raw to CM H20
const float PRESS_SENSOR_MULTIPLIER =  0.1331;
const float PRESS_SENSOR_CONSTANT = -5.7;


// control variables
float inspPressure =  INSP_PRESS_DEFAULT;
int respRate = INSP_PRESS_DEFAULT;
int tidal = TIDAL_DEFAULT;
float iERatio = I_E_RATIO_DEFAULT;

// updated control variables ..
// Control parameters must only change at the end of a breath cycle
// We can't have rates and values changing half-way through a breath and in an inconsistent manner
// Therefore when the control parameters are updated they are stored in this variables, then 'swapped in' at the end of a stroke
// ALSO .. we have to be careful about race-conditions for any shared variables. We can't have the interrupt driven control loop
// using a variable that has been partially updated by the code that was interrupted
// Therefore the use of a 'paramUpdateSemaphor' and a halting of interrupts to ensure that parameter updating is synchronised
int newInspPressure =  INSP_PRESS_DEFAULT;
int newRespRate = INSP_PRESS_DEFAULT;
int newTidal = TIDAL_DEFAULT;
float newIERatio = I_E_RATIO_DEFAULT;
volatile bool paramUpdateSemaphore;                  // TRUE -> Control parameters have been updated, reset to FALSE on use


// Physical set up constants
const short int NUM_OF_LEDS = 3;      // Count of LEDs on the box
const int INHALE_LED = 5;             // Blue LED - ( LEDG_PIN_VIN on circuito.io )
const int EXHALE_LED = 6;             // Green LED - ( LEDR_PIN_VIN on circuito.io )
const int SPONTANEOUS_LED = 7;        // Yellow LED
const int ROTARY_CLK = 2;             // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
const int ROTARY_DT = 3;              // Connected to DT on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
const int SELECT_BUTTON = 4;          // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )
const int PWM_PIN = 9;                // Pin for the final output
const int PRESSURE_SENSOR_PIN = A0;   // Pin for the pressure sensor
const int SERIAL_BAUD_RATE = 9600;    // Standard baud rate for the serial interface
const short int NUM_LED_TEST_LOOPS = 38; // Number of times that the LEDs flash during start-up test

unsigned long changeTime;

int pinALast;      // last pin high on rotary encoder
int ledFlashCount = 0; // only for debugging

const long int TIME_BETWEEN_TICKS = 10000;                           // Time between the main-control interrupt being called in microseconds
const long int TICKS_PER_MINUTE = 60000000 / TIME_BETWEEN_TICKS;       // assume milli-second clock tick .. calibrate to clock speed

int ticksPerInhale = 0;                    // Number of clock-ticks per inhale
int ticksPerExhale = 0;                    // Nmber of clock-ticks per exhale
int tick = 0;                             // One tick each time the controller 'main-loop' interrupt is fired
int breathState = INHALE_STATE;           // starts with machine driving breathing
int positionInDriveTable = 0;             // the offset down the output drive table for current clock tick
int unscaledDriveValue = 0;               // Drive value before being scalled by tidal volume
int driveValue = 0;                       // The value output to the actuator

String outPutLog = "";

// Constants and variables associated with different operating modes#
const int NUMBER_OF_MODES = 2;                // There are two modes .. 'IPPV' (Intermittent Positive Pressure Ventilaition) and 'Spontaneous'
const int MODE_IPPV = 0;
const int MODE_SPONTANEOUS = 1;
const int MODE_MAX_STRING_LEN = 12;           // Maximum length of a string representing a mode
char modeStrings [NUMBER_OF_MODES] [MODE_MAX_STRING_LEN] = {
  {"IPPV       "},
  {"Spontaneous"}
};
int currentMode = MODE_IPPV;
int spontLEDFlashCount = 0;                  // A counter that causes the spontaneous mode LED to flash
const int SPONT_LED_FLASH_START = 20;        // Number of times around the main control loop that spontaneous LED stays illuminated for
const int SPONT_LED_FLASH_FINISH = 40;       // Total times around the main loop before the spontLEDFlashCount is reset

const float SPONT_PRESSURE_THRESHOLD = 5.0;          // The threshold below which a breath is initiated in spontaneous mode

// Declare forward references ???!?!??!
void updateDisplay();
void calcTicksPerCycle(int respRate, float iERatio );
void ventControlInterrupt();
void calibrate();
void changeSettings();
float getKnob(float settingMin, float settingMax, float currentVal, float setStep, const String & units_str);
int getMode(int currentMode);

void setup()
{
  pinMode(INHALE_LED, OUTPUT);            // These are the red and green LEDS that indicate the breathing state
  pinMode(EXHALE_LED, OUTPUT);
  digitalWrite(INHALE_LED, LOW);
  digitalWrite(EXHALE_LED, LOW);

  //Initialize serial and wait for port to open:
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  for (int thisPressReading = 0; thisPressReading< PRESS_READ_SMOOTHING; thisPressReading++) {
    pressReadings[ thisPressReading ];
  }


  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();
  if (not PRODUCTION_CODE )  {
    lcd.setCursor(0, 0);
    lcd.print("Test software only");
    lcd.setCursor(0, 1);
    lcd.print("Not for medical use");
    lcd.setCursor(0,3);
    lcd.print("Software version V24");
    for (int i = 1; i<=NUM_LED_TEST_LOOPS; i++) {                    // Just a visible self-test to show that all of the LEDs are working
      digitalWrite(INHALE_LED,(i % NUM_OF_LEDS));
      digitalWrite(EXHALE_LED,((i+1) % NUM_OF_LEDS));
      digitalWrite(SPONTANEOUS_LED, ((i+2) % NUM_OF_LEDS));
      delay(100);
    }
    delay(2000);
    lcd.clear();
  }
  updateDisplay();

  pinMode(SELECT_BUTTON, INPUT);        // input from centre button of the rotary encoder
  pinMode (ROTARY_CLK, INPUT);               // Input from CLK of rotary encoder
  pinMode (ROTARY_DT, INPUT);               // Input from DT of rotary encoder
  pinALast = digitalRead(ROTARY_CLK);        // Remember the state of the rotary encoder CLK

  calcTicksPerCycle(respRate, iERatio );     // Update breath tick rates based on default RR and I.E.

  changeTime = millis();
  breathState = INHALE_STATE;
  Serial.println("Motor to squeeze BVM");

  paramUpdateSemaphore = false;               // determines access to the control parameters. Set to TRUE when the parameters have been updated

  Timer1.initialize(TIME_BETWEEN_TICKS);                        // Set the timer interrupt
  Timer1.attachInterrupt(ventControlInterrupt, TIME_BETWEEN_TICKS);    // Call the ventilator control loop 100 times per second (probably does not need to be that fast! )


  Timer1.pwm(PWM_PIN, DRIVE_VAL_MIN); // By default -set the controller to 'fully open'

}

void loop() {
  int selectButtonData = digitalRead(SELECT_BUTTON);
  // Serial.println(selectButtonData);
  if (selectButtonData == LOW && (millis() - changeTime) > 500) {
    changeSettings();
  }

  // Now read the pressure from the sensor
 // subtract the last reading:
  pressTotal = pressTotal - pressReadings[pressReadIndex];
  // read from the sensor:
  pressReadings[ pressReadIndex ] = analogRead( PRESSURE_SENSOR_PIN );
  // add the reading to the total:
  pressTotal = pressTotal + pressReadings[ pressReadIndex ];
  // advance to the next position in the array:
  pressReadIndex = pressReadIndex + 1;

  // if we're at the end of the array...
  if ( pressReadIndex >= PRESS_READ_SMOOTHING ) {
    // ...wrap around to the beginning:
    pressReadIndex = 0;
  }

  // calculate the average:
  int sensorValue = pressTotal / PRESS_READ_SMOOTHING;
  inspPressure = sensorValue * PRESS_SENSOR_MULTIPLIER + PRESS_SENSOR_CONSTANT ;                     // In this version measured rather than set

  if (inspPressure < 0) {
    inspPressure = 0;
  }
  static char pressStr[15];
  dtostrf(inspPressure,4,1,pressStr);
  lcd.setCursor(10, 0);
  lcd.print(pressStr);           // As measured by sensor
  lcd.setCursor(15,0);
  lcd.print("cmH2O");
  // Serial.println(pressStr);

  if (currentMode == MODE_SPONTANEOUS ) {
    spontLEDFlashCount++;                         // Increate the count for flashing the LED
    if (spontLEDFlashCount < SPONT_LED_FLASH_START) {
          digitalWrite(SPONTANEOUS_LED, HIGH);
    } else {
      digitalWrite(SPONTANEOUS_LED,LOW);
    }
    if (spontLEDFlashCount >= SPONT_LED_FLASH_FINISH) {
      spontLEDFlashCount = 0;
    }
  } else {
    digitalWrite(SPONTANEOUS_LED, LOW);
  }
}

void ventControlInterrupt() {

  tick = tick + 1;                               // This will be called, hence incremented every 'TIME_BETWEEN_TICKS' microseconds

  // If we are in a spontaneous breathing mode, and during an Exhale the measured pressure falls below PEEP
  // this means that the patient has sucked in air and therefore we need to trigger an inhalation
  if ((currentMode == MODE_SPONTANEOUS) &&  (breathState == EXHALE_STATE)) {
      int pressSensorRaw = analogRead( PRESSURE_SENSOR_PIN );                              // Get the instantaneous pressure
      float inspPressure = pressSensorRaw * PRESS_SENSOR_MULTIPLIER + PRESS_SENSOR_CONSTANT;  // Convert to CM H20 (just for consistency)
      if (inspPressure <= SPONT_PRESSURE_THRESHOLD) {                                      // PRessure is below PEEP .. implies that the patient is trying to breath in
        tick = 0;
        breathState = INHALE_STATE;
        Serial.println("Patient triggered inhale");
      }
  } ;


  // Output the correct drive value to the actuator
  // This depends on if we are inhaling (breathState == INHALE_STATE) or exhaling (breathState = EXHALE_STATE)
  if (breathState == INHALE_STATE) {
    positionInDriveTable = int((long) DRIVE_TABLE_SIZE * (long) tick / ticksPerInhale);       // How far down the drive table for drive value at this time (tick)?
    if (positionInDriveTable > DRIVE_TABLE_SIZE) {
      positionInDriveTable = DRIVE_TABLE_SIZE;
    }
    unscaledDriveValue = INHALE_DRIVE[ positionInDriveTable ];
    driveValue = unscaledDriveValue * (float) tidal / (float) TIDAL_MAX;              // Scales the drive value as a proportion of the maximum allowed tidal volume

    Timer1.setPwmDuty(PWM_PIN, driveValue);                                           // PWM Output converted to 4-20mA control signal, externally

    digitalWrite(INHALE_LED, HIGH);
    digitalWrite(EXHALE_LED, LOW);

  } else {
    // by inference, we must be in the exhale state
    // positionInDriveTable = 0;

    Timer1.setPwmDuty(PWM_PIN, DRIVE_VAL_MIN);     // PWM Output converted to 4-20mA control signal externally

    digitalWrite(INHALE_LED, LOW);
    digitalWrite(EXHALE_LED, HIGH);

  }

  // Send this drive value to the PWM output ... or somewhere else ?
  // Serial.print("tick =");
  // Serial.println(tick);
  //Serial.print("Unscaled drive value = ");
  //Serial.println(unscaledDriveValue);
  //Serial.print("Drive value");
  //Serial.println(driveValue);


  // This section manages the state transition
  // Depending on which state we are in (INHALE_STATE or EXHALE_STATE) .. we look at the elapsed time, and determine if it is time to change state
  // If it is time to change state, then we change the state (as stored in 'breathState')

  if ((tick >= ticksPerInhale) && (breathState == INHALE_STATE))  {
    // Time to exhale
    Serial.println("    Exhale");
    breathState = EXHALE_STATE;              // switch to exhaling
    tick = 0;
  };
  if ((tick >= ticksPerExhale) && (breathState == EXHALE_STATE))  {
    // Time to Inhale
    Serial.println("Inhale");
    breathState = INHALE_STATE;              // switch to inhaling
    tick = 0;

    // At the end of each cycle, check to see if control parameters have been updated
    if (paramUpdateSemaphore == true) {            // Parameters have been updated, we need to use these on the next cycle
      // inspPressure = newInspPressure;
      respRate = newRespRate;
      tidal = newTidal;
      iERatio = newIERatio;
      paramUpdateSemaphore = false;         // Signal that we have finished updating the control parameters
    }
  }

}

void calcTicksPerCycle(int respRate, float iERatio ) {
  // Calculates the number of clock cycles per inhale, exhale

  int ticksPerBreath = TICKS_PER_MINUTE / respRate;
  float iPlusE = 1 + iERatio;
  ticksPerInhale = int ( ticksPerBreath / iPlusE);
  ticksPerExhale = int ( ticksPerBreath * (iERatio / iPlusE));
  //Serial.print("Ticks per inhale = ");
  //Serial.println(ticksPerInhale);

  //Serial.print("Ticks per exhale = ");
  //Serial.println(ticksPerExhale);

}


void updateDisplay() {
  // Turn on the blacklight and print a message.
  lcd.setCursor(0, 0);
  lcd.print(respRate);
  lcd.setCursor(4, 0);
  lcd.print("bpm |");

  //lcd.setCursor(12, 0);
  //lcd.print(inspPressure);           // As measured by sensor
  lcd.setCursor(15, 0);
  lcd.print("cmH20");

  lcd.setCursor(0, 1);
  lcd.print("________|___________");

  lcd.setCursor(0, 2);
  lcd.print("        |");

  lcd.setCursor(0, 3);
  lcd.print("1:");
  lcd.print(iERatio);
  lcd.setCursor(8, 3);
  lcd.print("|");

  lcd.setCursor(12, 3);
  lcd.print(tidal);
  lcd.setCursor(17, 3);
  lcd.print("ml");
}

void changeSettings() {

  lcd.clear();

  // We don't actualy update the control parameters directly .. this is done within the control loop at the end of a cycle
  // What we do here is to collect an updated set of parameters, and set a semaphore to indicate to the control loop to pick them up


  // Next section triggers device calibration .. but only for non-production (test code).
  if (not PRODUCTION_CODE) {
    long int timeSelected = millis();                                          // Counts the number of loops that the select button has to be pushed for, before going into calibration mode
    int selectButtonData = digitalRead(SELECT_BUTTON);
    if (selectButtonData == LOW) {
      lcd.setCursor(0, 0);
      lcd.print("Keep button pushed");
      lcd.setCursor(0, 1);
      lcd.print("to calibrate");
    }
    while ((selectButtonData == LOW) && ((millis() - timeSelected) < ENTER_CALIBRATION )) {
      selectButtonData = digitalRead(SELECT_BUTTON);
    }
    if ((selectButtonData == LOW) && ((millis() - timeSelected) >= ENTER_CALIBRATION )) {
      calibrate();
    }
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode:");
  currentMode = getMode(currentMode);


  // Serial.println("Respiratory rate");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Respiratory rate:");
  newRespRate = int(getKnob(RESP_RATE_MIN, RESP_RATE_MAX, respRate, RESP_RATE_STEP, "bpm"));

  // Serial.println("Inspiratory Pressure");               // Not implemented in this version
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Inspiratory Press:");
  // newInspPressure = int(getKnob(INSP_PRESS_MIN, INSP_PRESS_MAX, inspPressure, 1.0, "cmH20"));

  // Serial.println("I.E. Ratio");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("I.E. Ratio:");
  newIERatio = getKnob(I_E_RATIO_MIN, I_E_RATIO_MAX, iERatio, I_E_RATIO_STEP, "");

  // Serial.println("TIDAL");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TIDAL:");
  newTidal = int(getKnob(TIDAL_MIN, TIDAL_MAX, tidal, TIDAL_STEP, "ml"));

  // Now signal to the interrup-driven control loop that it can pick up the new control parameters when it is ready to do to (at the start of a cycle)
  Timer1.stop();                   // Halt the interrupt so that there is no chance of a race condition
  paramUpdateSemaphore = true;     // Inform the main control loop that it can pick up the new control parameters
  Timer1.start();                  // Re-start the main control-loop interrupt

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Adjusting ...");

  while (paramUpdateSemaphore) {};  // wait until this update has been picked up

  // Finished sending updated parameters to control loop


  lcd.clear();
  updateDisplay();

  calcTicksPerCycle(respRate, iERatio );           // Update the breath cycles based on the new settings of RR and I.E. ratio

  changeTime = millis(); // manage the keyboard de-bounce
}

float getKnob(float settingMin, float settingMax, float currentVal, float setStep, const String & units_str) {

  int selectButtonData = digitalRead(SELECT_BUTTON);
  int aVal;
  boolean bCW;

  // wait for the selection button to go get released again
  while (selectButtonData == LOW) {
    selectButtonData = digitalRead(SELECT_BUTTON);
    // Serial.println(selectButtonData);
  }

  while (selectButtonData == HIGH) {

    aVal = digitalRead(ROTARY_CLK);
    if (aVal != pinALast) { // Means the knob is rotating
      // if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (digitalRead(ROTARY_DT) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
        bCW = true;
      } else {// Otherwise B changed first and we're moving CCW
        bCW = false;
      }
      // Serial.print ("Rotated: ");
      if (bCW) {
        // Serial.println ("clockwise");
        if (currentVal < (settingMax - setStep)) {
          currentVal = currentVal + setStep;
        }
      } else {
        // Serial.println("counterclockwise");
        if (currentVal > (settingMin + setStep)) {
          currentVal = currentVal - setStep;
        }
      }
    }
    pinALast = aVal;

    lcd.setCursor(2, 2);
    lcd.print(currentVal);
    lcd.print("  ");
    lcd.setCursor(15, 2);
    lcd.print(units_str);
    selectButtonData = digitalRead(SELECT_BUTTON);
  }

  return (currentVal);
}

int getMode(int currentMode) {

  int selectButtonData = digitalRead(SELECT_BUTTON);
  int aVal;
  boolean bCW;

  // wait for the selection button to go get released again
  while (selectButtonData == LOW) {
    selectButtonData = digitalRead(SELECT_BUTTON);
    // Serial.println(selectButtonData);
  }

  while (selectButtonData == HIGH) {

    aVal = digitalRead(ROTARY_CLK);
    if (aVal != pinALast) { // Means the knob is rotating
      // if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (digitalRead(ROTARY_DT) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
        bCW = true;
      } else {// Otherwise B changed first and we're moving CCW
        bCW = false;
      }
      // Serial.print ("Rotated: ");
      if (bCW) {
        // Serial.println ("clockwise");
          currentMode++;
          if (currentMode >= NUMBER_OF_MODES) {
            currentMode = 0;
        }
      } else {
        // Serial.println("counterclockwise");
          currentMode--;
          if (currentMode <0) {
            currentMode = ( NUMBER_OF_MODES - 1);  // array index is zero based remember
        }
      }
    }
    pinALast = aVal;

    lcd.setCursor(2, 2);
    lcd.print(modeStrings[currentMode]);

    selectButtonData = digitalRead(SELECT_BUTTON);
  }

  return (currentMode);
}




void calibrate() {
  Timer1.stop();                   // Halt the main control interrupt - we are now in calibration mode
  Timer1.detachInterrupt();
  Timer1.pwm(PWM_PIN, RAW_ACTUATOR_MIN);

  int rawActuatorSetting  = RAW_ACTUATOR_MIN;             // set the actuator to its minumum value
  int lastRawActSetting = RAW_ACTUATOR_MIN;
  int setting = RAW_ACTUATOR_MIN;

  lcd.clear();
  lcd.print("Calibration mode");

  while (1) {                        // lock-up .. no exit from here without hard-reset!
    lcd.setCursor(0, 1);
    lcd.print("Raw actuator value:");
    rawActuatorSetting = int(getKnob(RAW_ACTUATOR_MIN, RAW_ACTUATOR_MAX, lastRawActSetting, RAW_ACTUATOR_STEP, "(raw)"));
    if (rawActuatorSetting > lastRawActSetting) {
      for (setting = lastRawActSetting; setting <= rawActuatorSetting; setting++) {
        Timer1.setPwmDuty(PWM_PIN, setting);
        delay(5);                                       // slow things down to remove abrupt changes
      }
    } else {
      for (setting = lastRawActSetting;  setting >= rawActuatorSetting; setting--) {
        Timer1.setPwmDuty(PWM_PIN, setting);
        delay(5);                                       // slow things down to remove abrupt changes
      }
    }
    lastRawActSetting = rawActuatorSetting;
    int pressureRaw = analogRead( PRESSURE_SENSOR_PIN );                 // Read the raw input from the pressure sensor
    lcd.setCursor(0,3);
    lcd.print("Raw pressure =      ");
    lcd.setCursor(16,3);
    lcd.print(pressureRaw);
  }
}
