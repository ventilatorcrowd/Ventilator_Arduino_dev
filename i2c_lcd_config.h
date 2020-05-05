#ifndef I2C_LCD_CONFIG_H
 #define I2C_LCD_CONFIG_H
 #ifdef I2C_LCD

   #include <LiquidCrystal_I2C.h>

   // Set the LCD address to 0x27 for a 16 chars and 2 line display
   LiquidCrystal_I2C lcd(0x27, 20, 4);
   
   const int LCD_BORDER     = 0;
   const int LCD_TOP_ROW    = 0;
   const int LCD_BOTTOM_ROW = 1;
   const int LCD_QUARTER_WIDTH   = 1;
   const int LCD_HALF_WIDTH   = 120;
 #endif
#endif
