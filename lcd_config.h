#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H
 #ifdef I2C_LCD
  #include "i2c_lcd_config.h"
 #else
  #ifdef PARALLEL_LCD
   #include "parallel_lcd_config.h"
  #endif
 #endif
#endif
