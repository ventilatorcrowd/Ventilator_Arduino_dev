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
const float I_E_RATIO_Min = 0.5;
const float I_E_RATIO_DEFAULT = 1;
const float I_E_RATIO_STEP = 0.2;

// Physical set up constants
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to

// control variables
int inspPressure =  INSP_PRESS_DEFAULT;
int respRate = INSP_PRESS_DEFAULT;
int peep = PEEP_DEFAULT;
float iERatio = I_E_RATIO_DEFAULT;

int selectButton = 2; // Input pin for push button
int ledState = 0; // 0 = 0ff, 1 = on
unsigned long changeTime;

int runState = 0;  // 0 = normal run, 1 = adjust RR, 2 = adjust Insp. Press, 3 = I.e. Ratio, 4 = PEEP, .. cycles back to 0

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

  pinMode(selectButton, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  ledState = 0;
  runState = 0; // set to normal runnig mode

  digitalWrite(LED_BUILTIN, LOW);
  changeTime = millis();
}

void loop()
{
  int selectButtonData = digitalRead(selectButton);
  // Serial.println(state);
  if (selectButtonData == LOW && (millis()-changeTime) > 500) {
    changeSettings();
  }
}

void updateDisplay() {
    // Turn on the blacklight and print a message.
  lcd.setCursor(0,0);
  lcd.print(respRate);
  lcd.setCursor(6,0);
  lcd.print("bpm |");

  lcd.setCursor(12,0);
  lcd.print(inspPressure);
  lcd.setCursor(15,0);
  lcd.print("cmH20");

  lcd.setCursor(0,1);
  lcd.print("__________|_________");

  lcd.setCursor(0,2);
  lcd.print("          |");

  lcd.setCursor(0,3);
  lcd.print("1:");
  lcd.print(iERatio);
  lcd.setCursor(10,3);
  lcd.print("|");
  lcd.setCursor(12,3);
  lcd.print(peep);
  lcd.setCursor(15,3);
  lcd.print("cmH20");
}



void changeSettings() {

  runState = runState + 1;   // move to next display set-up mode
  if (runState == 5) {
    runState = 0;
  }

  lcd.clear();
    int selectButtonData = digitalRead(selectButton);
    Serial.println(selectButtonData);
    
  switch (runState) {
    case 0:
      Serial.println("Normal running");
      updateDisplay();
      break;
    case 1:
      Serial.println("Respiratory rate");
      lcd.print("Respiratory rate:");
      respRate = getKnobInt(RESP_RATE_MIN, RESP_RATE_MAX, "bpm");
      break;
    case 2:
      Serial.println("Inspiratory Pressure");
      lcd.print("Inspiratory Press:");
      inspPressure = getKnobInt(INSP_PRESS_MIN, INSP_PRESS_MAX, "cmH20");
      break;
    case 3:
      Serial.println("I.E. Ratio");
      lcd.print("I.E. Ratio:");
      break;
    case 4:
      Serial.println("PEEP");
      lcd.print("PEEP:"); 
      peep = getKnobInt(PEEP_MIN, PEEP_MAX, "cmH20");     
      break;
  }

  changeTime = millis(); // manage the keyboard de-bounce
}

int getKnobInt(int settingMin, int settingMax, const String& units_str) {
  int sensorValue = 0;
  int setting = 0;

   int selectButtonData = digitalRead(selectButton);

  // wait for the selection button to go get released again
  while (selectButtonData == LOW) {
   selectButtonData = digitalRead(selectButton); 
   Serial.println(selectButtonData);
  }
  
  while (selectButtonData == HIGH) {

    sensorValue = analogRead(analogInPin);
    setting = map(sensorValue, 0, 1023, settingMin, settingMax);
    lcd.setCursor(2,2);
    lcd.print(setting);
    lcd.print("  ");
    lcd.setCursor(15,2);
    lcd.print(units_str);
    selectButtonData = digitalRead(selectButton); 
  }   

  return(setting);
}
