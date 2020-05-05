#ifdef PARALLEL_LCD

void initialiseLcd()
{
   lcd.reset();
    
   uint16_t identifier = lcd.readID();
   if(identifier == 0x9325) 
   {
      Serial.println(F("Found ILI9325 LCD driver"));
   }
   else if(identifier == 0x9328)
   {
      Serial.println(F("Found ILI9328 LCD driver"));
   }
   else if(identifier == 0x4535)
   {
      Serial.println(F("Found LGDP4535 LCD driver"));
   }
   else if(identifier == 0x7575)
   {
      Serial.println(F("Found HX8347G LCD driver"));
   }
   else if(identifier == 0x9341)
   {
      Serial.println(F("Found ILI9341 LCD driver"));
   }
   else if(identifier == 0x7783)
   {
      Serial.println(F("Found ST7781 LCD driver"));
   }
   else if(identifier == 0x8230)
   {
      Serial.println(F("Found UC8230 LCD driver"));  
   }
   else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
   }
   else if(identifier==0x0101)
   {     
      identifier=0x9341;
      Serial.println(F("Found 0x9341 LCD driver"));
   }
   else if(identifier==0x9481)
   {     
      Serial.println(F("Found 0x9481 LCD driver"));
   }
   else if(identifier==0x9486)
   {     
      Serial.println(F("Found 0x9486 LCD driver"));
   }
   else
   {
      Serial.print(F("Unknown LCD driver chip: "));
      Serial.println(identifier, HEX);
      Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
      Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
      Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
      Serial.println(F("If using the breakout board, it should NOT be #defined!"));
      Serial.println(F("Also if using the breakout, double-check that all wiring"));
      Serial.println(F("matches the tutorial."));
      identifier=0x9486;
   }
  
   lcd.begin(identifier);
   lcd.setRotation(1);
   lcd.fillScreen(BLUE);
   lcd.setTextColor(WHITE);
   lcd.setTextSize(4);
  
   Serial.print("TFT size is "); Serial.print(lcd.width()); Serial.print("x"); Serial.println(lcd.height());
}

void clearDisplay()
{
   lcd.fillScreen(BLUE);
}

void updateDisplay()
{
    lcd.fillRect(0, LCD_HALF_HEIGHT, LCD_WIDTH, 5, WHITE);
    lcd.fillRect(LCD_HALF_WIDTH, 0, 5,LCD_HEIGHT, WHITE);

    lcd.setCursor(LCD_BORDER, LCD_TOP_ROW);
    lcd.print(respRate);
    lcd.setCursor(LCD_QUARTER_WIDTH, LCD_TOP_ROW);
    lcd.print("bpm");

    lcd.setCursor((LCD_HALF_WIDTH) + LCD_BORDER, LCD_TOP_ROW);
    lcd.print(inspPressure);
//    lcd.setCursor((LCD_QUARTER_WIDTH) + (LCD_HALF_WIDTH), LCD_TOP_ROW);
//    lcd.print("cmH20");

    lcd.setCursor(LCD_BORDER, LCD_BOTTOM_ROW);
    lcd.print("1:");
    lcd.print(iERatio);

    lcd.setCursor((LCD_HALF_WIDTH) + LCD_BORDER, LCD_BOTTOM_ROW);
    lcd.println(tidal);
    lcd.setCursor((LCD_QUARTER_WIDTH) + (LCD_HALF_WIDTH), LCD_BOTTOM_ROW);
    lcd.println("ml");
}
#endif
