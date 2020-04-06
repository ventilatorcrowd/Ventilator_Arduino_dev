#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3f, 20, 4);

// define control constants for the ventilator
const int INSP_PRESS_MAX = 20;           // Inspiratory pressure
const int INSP_PRESS_MIN = 5;
const int INSP_PRESS_DEFAULT = 15;
const int INSP_PRESS_STEP = 1;

const int RESP_RATE_MAX = 30;             // Respratory rate
const int RESP_RATE_MIN = 10;
const int RESP_RATE_DEFAULT = 20;
const int RESP_RATE_STEP = 1;

const int PEEP_MAX = 7;                   // PEEP
const int PEEP_MIN = 1;
const int PEEP_DEFAULT = 2;
const float PEP_STEP = 0.2;

const float I_E_RATIO_MAX = 3.0;           // Inspiratory - expiratory ratio
const float I_E_RATIO_MIN = 0.5;
const float I_E_RATIO_DEFAULT = 1;
const float I_E_RATIO_STEP = 0.2;

const int INHALE_STATE = 0;     // vent is driving inhalation
const int EXHALE_STATE = 1;      // vent is driving exhlation

// control variables
int inspPressure =  INSP_PRESS_DEFAULT;
int respRate = INSP_PRESS_DEFAULT;
int peep = PEEP_DEFAULT;
float iERatio = I_E_RATIO_DEFAULT;

// Physical set up constants
const int selectButton = 0; // // Push switch built into the rotary encoder
const int pinA = 3;  // Connected to CLK on KY-040
const int pinB = 4;  // Connected to DT on KY-040

int ledState = 0; // 0 = 0ff, 1 = on
unsigned long changeTime;

int runState = 0;  // 0 = normal run, 1 = adjust RR, 2 = adjust Insp. Press, 3 = I.e. Ratio, 4 = PEEP, .. cycles back to 0
int pinALast;      // last pin high on rotary encoder

const long int TICKS_PER_MINUTE = 60000;         // assume milli-second clock tick .. calibrate to clock speed
int ticksPerInhale = 0;                    // Number of clock-ticks per inhale
int ticksPerExhale = 0;                    // Nmber of clock-ticks per exhale
int breathClock = 0;                      // Counts clock-ticks for the breath cycle
int breathState = INHALE_STATE;           // starts with machine driving breathing
int elapsed = 0;

void setup()
{
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

  pinMode(selectButton, INPUT);        // input from centre button of the rotary encoder
  // pinMode(LED_BUILTIN, OUTPUT);
  pinMode (pinA, INPUT);               // Input from CLK of rotary encoder
  pinMode (pinB, INPUT);               // Input from DT of rotary encoder
  pinALast = digitalRead(pinA);        // Remember the state of the rotary encoder CLK

  ledState = 0;
  runState = 0; // set to normal running mode

  calcTicksPerCycle(respRate, iERatio );     // Update breath tick rates based on default RR and I.E.

  digitalWrite(LED_BUILTIN, LOW);
  changeTime = millis();
  breathClock = millis();
  breathState = INHALE_STATE;
  Serial.println("Motor to squeeze BVM");
}

void loop()
{
  int selectButtonData = digitalRead(selectButton);
  // Serial.println(selectButtonData);
  if (selectButtonData == LOW && (millis() - changeTime) > 500) {
    changeSettings();
  }


  elapsed = millis() - breathClock;   // how many clock ticks have passed?
  if ((elapsed >= ticksPerInhale) && (breathState == INHALE_STATE))  {
    // Time to exhale
    Serial.println("Motor to release BVM");
    breathState = EXHALE_STATE;              // switch to exhaling
    printBreathIndicator(0);
    breathClock = millis();                  // reset the clock
    elapsed = 0;
  };
  if ((elapsed >= ticksPerExhale) && (breathState == EXHALE_STATE))  {
    // Time to Inhale
    Serial.println("Motor to squeeze BVM");
    breathState = INHALE_STATE;              // switch to inhaling
    printBreathIndicator(20);
    breathClock = millis();                  // reset the clock
  }
}

void calcTicksPerCycle(int respRate, float iERatio ) {
  // Calculates the number of clock cycles per inhale, exhale

  int ticksPerBreath = TICKS_PER_MINUTE / respRate;
  ticksPerInhale = int ( ticksPerBreath * ( 1 / (1 + iERatio)));
  ticksPerExhale = int ( ticksPerBreath * (iERatio / (1 + iERatio)));

  Serial.print("Ticks per Inhale = ");
  Serial.println(ticksPerInhale);
  Serial.print("Ticks per Exhale = ");
  Serial.println(ticksPerExhale);

}


void updateDisplay() {
  // Turn on the blacklight and print a message.
  lcd.setCursor(0, 0);
  lcd.print(respRate);
  lcd.setCursor(6, 0);
  lcd.print("bpm |");

  lcd.setCursor(12, 0);
  lcd.print(inspPressure);
  lcd.setCursor(15, 0);
  lcd.print("cmH20");

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
  lcd.print(peep);
  lcd.setCursor(15, 3);
  lcd.print("cmH20");
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

  // Serial.println("Respiratory rate");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Respiratory rate:");
  respRate = int(getKnob(RESP_RATE_MIN, RESP_RATE_MAX, respRate, 1.0, "bpm"));

  // Serial.println("Inspiratory Pressure");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Inspiratory Press:");
  inspPressure = int(getKnob(INSP_PRESS_MIN, INSP_PRESS_MAX, inspPressure, 1.0, "cmH20"));

  // Serial.println("I.E. Ratio");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("I.E. Ratio:");
  iERatio = getKnob(I_E_RATIO_MIN, I_E_RATIO_MAX, iERatio, 0.2, "");

  // Serial.println("PEEP");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PEEP:");
  peep = int(getKnob(PEEP_MIN, PEEP_MAX, peep, 1.0, "cmH20"));

  lcd.clear();
  updateDisplay();

  calcTicksPerCycle(respRate, iERatio );           // UPdate the breath cycles based on the new settings of RR and I.E. ratio

  changeTime = millis(); // manage the keyboard de-bounce
}

float getKnob(int settingMin, int settingMax, float currentVal, float setStep, const String& units_str) {
  int sensorValue = 0;
  int selectButtonData = digitalRead(selectButton);
  int aVal;
  boolean bCW;

  // wait for the selection button to go get released again
  while (selectButtonData == LOW) {
    selectButtonData = digitalRead(selectButton);
    // Serial.println(selectButtonData);
  }

  while (selectButtonData == HIGH) {

    aVal = digitalRead(pinA);
    if (aVal != pinALast) { // Means the knob is rotating
      // if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (digitalRead(pinB) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
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
    selectButtonData = digitalRead(selectButton);
  }

  return (currentVal);
}
