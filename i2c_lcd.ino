#ifdef I2C_LCD
#include <Wire.h>

void initialiseLcd()
{
  Wire.begin(); // Start I2C.
  lcd.begin();
  lcd.backlight();
}

void clearDisplay()
{
  lcd.clear();
}

void updateDisplay() {
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
#endif
