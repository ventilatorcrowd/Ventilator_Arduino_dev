// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.
//Technical support:goodlcd@163.com

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "Controller.h"

void clearDisplay( void );

void textDisplay( U16 x, U16 y, const S8 * string, bool newLine );

void numDisplay( U16  x, U16  y, U16  num, bool newLine );

void updateDisplay( void );

void changeDisplaySettings( void );

void printBreathIndicator( U16  numBlocks ); 

void setupDisplay ( void ) ;

void DisplayVersionNum ( void ) ;

void DisplaySetUpValue ( U16  value );


void DisplaySetUpMsg( S8 *msg );

#endif
