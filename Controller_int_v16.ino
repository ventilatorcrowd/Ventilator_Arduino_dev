/* Author      : Rob Collins
 * Version     : 15
 * Description : Simple ventilator controller providing a simple human interface for Respiratory Rate, I.E. Ratio and Tidal volume
 *               Ventilator control cycle runs from a timer interrupt .. enabling control parameters to be changed whilst the controler
 *               continues to provide control signals
 * Changes     : V15 : Introduces intermediate variables and a 'semaphor' between the parameter change code and the main control loop
 *                     This is to enable changed parameters to only take effect at the end of a breathing cycle.
 *                     Also avoid the problem of a race condition in the case where parameter variables are partially update when a control-loop interrupt occures
 *                     This version uses a 'semaphor' to signal to the main control loop that parameters have been updated. This semaphor is only set whilst the
 *                     interrupt timers are halted
 *                     
 *             : V16 : This version has outpur pins re-assigned to match the circuit layout defined using circutio.io
 * Circuit     : https://www.circuito.io/app?components=512,9590,9591,11021,217614,417987
 *             : Also add a 10k pull-up resistor between Rotary centre switch (Middle pin, 'SELECT_BUTTON' ) and +5V .. this device has no built in pull-up for centre switch
 * Status      : Untested
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>                        // Interrupt library to keep the control cycle regular .. https://playground.arduino.cc/Code/Timer1/ 

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3f, 20, 4);

// Define shape of driver waveform
const int INHALE_DRIVE[] = {0, 1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 22, 23, 25, 26, 27, 28, 30, 31, 32, 33, 35, 36, 37, 38, 40, 41, 42, 43, 45, 46, 47, 48, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200, 205, 210, 215, 220, 225, 230, 235, 240, 245, 250, 252, 255, 257, 260, 262, 265, 267, 270, 272, 275, 277, 280, 282, 285, 287, 290, 292, 295, 297, 300};
const int DRIVE_TABLE_SIZE = 100;            // length of the INHALE_DRIVE table
const int DRIVE_VAL_MIN = 0;                 // minimum value for the output drive (position during exhale)

// define control constants for the ventilator
const int INSP_PRESS_MAX = 20;           // Inspiratory pressure
const int INSP_PRESS_MIN = 5;
const int INSP_PRESS_DEFAULT = 15;
const int INSP_PRESS_STEP = 1;

const int RESP_RATE_MAX = 30;             // Respratory rate
const int RESP_RATE_MIN = 10;
const int RESP_RATE_DEFAULT = 20;
const int RESP_RATE_STEP = 1;

const int TIDAL_MAX = 300;                   // TIDAL
const int TIDAL_MIN = 200;
const int TIDAL_DEFAULT = 250;
const int TIDAL_STEP = 10;

const float I_E_RATIO_MAX = 3.0;           // Inspiratory - expiratory ratio
const float I_E_RATIO_MIN = 0.2;
const float I_E_RATIO_DEFAULT = 1;
const float I_E_RATIO_STEP = 0.2;

const int INHALE_STATE = 0;     // vent is driving inhalation
const int EXHALE_STATE = 1;      // vent is driving exhlation

// control variables
int inspPressure =  INSP_PRESS_DEFAULT;
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
const int INHALE_LED = 5;             // GReen LED - ( LEDG_PIN_VIN on circuito.io )
const int EXHALE_LED = 6;             // Red LED - ( LEDR_PIN_VIN on circuito.io )
const int ROTARY_CLK = 2;             // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
const int ROTARY_DT = 3;              // Connected to DT on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
const int SELECT_BUTTON = 4;          // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )

int ledState = 0; // 0 = 0ff, 1 = on
unsigned long changeTime;

int runState = 0;  // 0 = normal run, 1 = adjust RR, 2 = adjust Insp. Press, 3 = I.e. Ratio, 4 = TIDAL, .. cycles back to 0
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

void setup()
{
  pinMode(INHALE_LED, OUTPUT);            // These are the red and green LEDS that indicate the breathing state
  pinMode(EXHALE_LED, OUTPUT);
  digitalWrite(INHALE_LED, LOW);
  digitalWrite(EXHALE_LED, LOW);

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  updateDisplay();

  pinMode(SELECT_BUTTON, INPUT);        // input from centre button of the rotary encoder
  pinMode (ROTARY_CLK, INPUT);               // Input from CLK of rotary encoder
  pinMode (ROTARY_DT, INPUT);               // Input from DT of rotary encoder
  pinALast = digitalRead(ROTARY_CLK);        // Remember the state of the rotary encoder CLK

  ledState = 0;
  runState = 0; // set to normal running mode

  calcTicksPerCycle(respRate, iERatio );     // Update breath tick rates based on default RR and I.E.

  digitalWrite(LED_BUILTIN, LOW);
  changeTime = millis();
  breathState = INHALE_STATE;
  Serial.println("Motor to squeeze BVM");

  paramUpdateSemaphore = false;               // determines access to the control parameters. Set to TRUE when the parameters have been updated

  Timer1.initialize(TIME_BETWEEN_TICKS);                        // Set the timer interrupt
  Timer1.attachInterrupt(ventControlInterrupt, TIME_BETWEEN_TICKS);    // Call the ventilator control loop 100 times per second (probably does not need to be that fast! )

}

void loop()
{
  int selectButtonData = digitalRead(SELECT_BUTTON);
  // Serial.println(selectButtonData);
  if (selectButtonData == LOW && (millis() - changeTime) > 500) {
    changeSettings();
  }

}

void ventControlInterrupt() {

  tick = tick + 1;                               // This will be called, hence incremented every 'TIME_BETWEEN_TICKS' microseconds

  //ledFlashCount = ledFlashCount + 1;
  //if (ledFlashCount < 10) {
  //  digitalWrite(INHALE_LED, HIGH);
  //  digitalWrite(EXHALE_LED, LOW);
  //}
  //if ((ledFlashCount > 10) && (ledFlashCount < 20)) {
  //  digitalWrite(INHALE_LED, LOW);
  //  digitalWrite(EXHALE_LED, HIGH);
  //}
  //if (ledFlashCount == 21) {
  //  ledFlashCount = 0;
  //}


  // First section outputs the correct drive value to the actuator
  // This depends on if we are inhaling (breathState == INHALE_STATE) or exhaling (breathState = EXHALE_STATE)
  if (breathState == INHALE_STATE) {
    positionInDriveTable = int((long) DRIVE_TABLE_SIZE * (long) tick / ticksPerInhale);       // How far down the drive table for drive value at this time (tick)?
    if (positionInDriveTable > DRIVE_TABLE_SIZE) {
      positionInDriveTable = DRIVE_TABLE_SIZE;
    }
    unscaledDriveValue = INHALE_DRIVE[ positionInDriveTable ];
    driveValue = unscaledDriveValue * (float) tidal / (float) TIDAL_MAX;              // Scales the drive value as a proportion of the maximum allowed tidal volume
    digitalWrite(INHALE_LED, HIGH);
    digitalWrite(EXHALE_LED, LOW);

  } else {
    // by inference, we must be in the exhale state
    positionInDriveTable = 0;
    driveValue = DRIVE_VAL_MIN;                      // No control over the exhale control waveform - patient air vents via a valve to external air
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


  // Second section manages the state transition
  // Depending on which state we are in (INHALE_STATE or EXHALE_STATE) .. we look at the elapsed time, and determine if it is time to change state
  // If it is time to change state, then we change the state (as stored in 'breathState')

  if ((tick >= ticksPerInhale) && (breathState == INHALE_STATE))  {
    // Time to exhale
    Serial.println("ON");
    breathState = EXHALE_STATE;              // switch to exhaling
    // printBreathIndicator(0);
    tick = 0;
  };
  if ((tick >= ticksPerExhale) && (breathState == EXHALE_STATE))  {
    // Time to Inhale
    Serial.println("OFF");
    breathState = INHALE_STATE;              // switch to inhaling
    // printBreathIndicator(20);
    tick = 0;

    // At the end of each cycle, check to see if control parameters have been updated
    if (paramUpdateSemaphore == true) {            // Parameters have been updated, we need to use these on the next cycle
      inspPressure = newInspPressure;
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
    Serial.print("Ticks per inhale = ");
    Serial.println(ticksPerInhale);

    Serial.print("Ticks per exhale = ");
    Serial.println(ticksPerExhale);

  }


  void updateDisplay() {
    // Turn on the blacklight and print a message.
    lcd.setCursor(0, 0);
    lcd.print(respRate);
    lcd.setCursor(6, 0);
    lcd.print("bpm |");

    lcd.setCursor(12, 0);
    lcd.print("-------");
    //lcd.print(inspPressure);           // Not implemented in this version
    //lcd.setCursor(15, 0);
    //lcd.print("cmH20");

    lcd.setCursor(0, 1);
    lcd.print("__________|_________");

    lcd.setCursor(0, 2);
    lcd.print("          |");

    lcd.setCursor(0, 3);
    lcd.print("1:");
    lcd.print(iERatio);
    lcd.setCursor(10, 3);
    lcd.print("|");

    lcd.setCursor(12, 3);
    lcd.print(tidal);
    lcd.setCursor(17, 3);
    lcd.print("ml");
  }

  void printBreathIndicator( int numBlocks) {

    lcd.setCursor(0, 1);
    for (int i = 1; i <= numBlocks; i++) {
      lcd.print("#");
    }
    for (int i = numBlocks + 1; i <= 20; i++) {
      lcd.print("_");
    }
  }


  void changeSettings() {

    lcd.clear();

    // We don't actualy update the control parameters directly .. this is done within the control loop at the end of a cycle
    // What we do here is to collect an updated set of parameters, and set a semaphore to indicate to the control loop to pick them up

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
    int sensorValue = 0;
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
          if (currentVal < settingMax ) {
            currentVal = currentVal + setStep;
          }
        } else {
          // Serial.println("counterclockwise");
          if (currentVal > settingMin) {
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
