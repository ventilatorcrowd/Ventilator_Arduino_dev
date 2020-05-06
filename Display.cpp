/*----------------------------------------------------------------------------\
|          Ventilator Crown Open Source Software                              |
|                                                                             |
|-----------------------------------------------------------------------------|
|                                                                             |
|   Module Name  : Display                                                    |
|                                                                             |
|   Author Name  : Paul Hughes                                                |
|                                                                             |
|   Date         : 01/5/2020                                                  |
|                                                                             |
|-----------------------------------------------------------------------------|
|                                                                             |
|                                                                             |
|-----------------------------------------------------------------------------|
|                                                                             |
|   Generic LCD Display functions                                             |
|                                                                             |
\----------------------------------------------------------------------------*/


// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.
//Technical support:goodlcd@163.com

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <MCUFRIEND_kbv.h>
#include "motor.h"
#include "Display.h"
#include "rotor_leds.h"

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define LCD_WIDTH           480
#define LCD_HEIGHT          320
#define LCD_HALF_WIDTH      240
#define LCD_QUARTER_WIDTH   120
#define LCD_HALF_HEIGHT     160
#define LCD_TOP_ROW          64
#define LCD_BOTTOM_ROW      224
#define LCD_BORDER           20

/* set up areas of the display */

/* 
 * banner at bootom of display for warnings and errors 
 */
#define BANNER_WIDTH        LCD_WIDTH
#define BANNER_HEIGHT              64
#define BANNER_TOP_ROW      ( LCD_HEIGHT - BANNER_HEIGHT )

#define BANNER_ERR_COLOUR   RED
#define BANNER_WARN_COLOUR  MAGENTA


#define MSG_UPDATE_X  0
#define MSG_UPDATE_Y  0
#define MSG_UPDATE_WIDTH  LCD_WIDTH
#define MSG_UPDATE_HIGHT  100

#define NUM_UPDATE_X  180
#define NUM_UPDATE_Y  130
#define NUM_UPDATE_WIDTH  120
#define NUM_UPDATE_HIGHT  60

#define NUM_LOC_UPDATE_X     200
#define NUM_LOC_UPDATE_Y     140

#define MSG_LOC_UPDATE_X     60
#define MSG_LOC_UPDATE_Y     40


MCUFRIEND_kbv lcd;

// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD lcd;


/*---------------------------------------------------------------------------------
 *  Code blocks untill button 1 pressed
 *
 *--------------------------------------------------------------------------------*/
void DisplayVersionNum ( void )
{
    Serial.println( "updateDisplay" );
  
    lcd.setRotation( 1 );
    lcd.fillScreen( BLUE );
    
    lcd.setTextColor( WHITE );
    lcd.setTextSize( 4);
  
    lcd.setCursor( LCD_BORDER, LCD_TOP_ROW );
    lcd.print( "Version Number " );
    lcd.print( VERSION_NUM_MAJOR );
    lcd.print( ":" );
    lcd.print( VERSION_NUM_MINOR );

    while ( getButtonsPressed() != BUT1_PRESSED )
    {
      delay(100);
    }
  
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void setupDisplay ( void ) 
{
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));

  lcd.reset();

  uint16_t identifier = lcd.readID();
  if(identifier == 0x9325) 
  {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) 
  {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) 
  {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) 
  {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) 
  {
    Serial.println(F("Found ILI9341 LCD driver"));
  }else if(identifier == 0x7783) 
  {
    Serial.println(F("Found ST7781 LCD driver"));
  }else if(identifier == 0x8230) 
  {
    Serial.println(F("Found UC8230 LCD driver"));  
  }
  else if(identifier == 0x8357) 
  {
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
  else {
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
  Serial.print("TFT size is "); 
  Serial.print(lcd.width()); 
  Serial.print("x"); 
  Serial.println(lcd.height());

}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void printBreathIndicator( U16 numBlocks ) 
{

  lcd.setCursor(0, 1);

  for (int i = 1; i <= numBlocks; i++) 
  {
    lcd.print("#");
  }
  for (int i = numBlocks + 1; i <= 20; i++) 
  {
    lcd.print("_");
  }

}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void clearDisplay( void )
{
    lcd.setRotation( 1 );
    lcd.fillScreen( BLUE );
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void textDisplay( U16 x, U16 y, const S8 * string , bool newLine )
{

    lcd.setCursor( x, y );
    if ( newLine == true )
    {
        lcd.println( string );
    }
    else
    {
        lcd.print( string );
    }

}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
static void blankNumUpdateArea ( void  )
{
    lcd.fillRect( NUM_UPDATE_X, NUM_UPDATE_Y, NUM_UPDATE_WIDTH, NUM_UPDATE_HIGHT, BLUE );
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
static void blankMsgUpdateArea ( void  )
{
    lcd.fillRect( MSG_UPDATE_X, MSG_UPDATE_Y, MSG_UPDATE_WIDTH, MSG_UPDATE_HIGHT, BLUE );
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void DisplaySetUpValue ( U16 value )
{

    // clear space for updated number
    blankNumUpdateArea();
    lcd.setTextSize( 4 );
    numDisplay( NUM_LOC_UPDATE_X, NUM_LOC_UPDATE_Y, value , false );
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void DisplaySetUpMsg ( S8 * msg )
{

    // clear space for updated number
    blankMsgUpdateArea();
    lcd.setTextColor( WHITE );
    lcd.setTextSize( 3 );
    textDisplay( MSG_LOC_UPDATE_X, MSG_LOC_UPDATE_Y, msg , false );
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void numDisplay( U16 x, U16 y, U16 num , bool newLine)
{

    lcd.setCursor( x, y );
    if ( newLine == true )
    {
        lcd.println( num );
    }
    else
    {
        lcd.print( num );
    }
}


// need to display numbers in active zones
// modified values displayed in change window  in center  


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void updateDisplay()
{
    Serial.println( "updateDisplay" );
  
    lcd.setRotation( 1 );
    lcd.fillScreen( BLUE );
    
    lcd.fillRect( 0, LCD_HALF_HEIGHT, LCD_WIDTH, 5, WHITE );
    lcd.fillRect( LCD_HALF_WIDTH, 0, 5,LCD_HEIGHT, WHITE );

    lcd.setTextColor( WHITE );
    lcd.setTextSize( 4);
  
    lcd.setCursor( LCD_BORDER, LCD_TOP_ROW );
    lcd.print( getSysValue( E_RESP_RATE ) );

    lcd.setCursor( LCD_QUARTER_WIDTH, LCD_TOP_ROW );
    lcd.print( "bpm" );

#if 1
    lcd.setCursor( ( LCD_QUARTER_WIDTH ) + ( LCD_HALF_WIDTH ), LCD_TOP_ROW );
    lcd.println( "-" );
#else
    lcd.setCursor( ( LCD_HALF_WIDTH ) + LCD_BORDER, LCD_TOP_ROW );
    lcd.print( getSysValue( E_RESP_RATE E_INSP_PRESS) );

    lcd.setCursor( ( LCD_QUARTER_WIDTH ) + ( LCD_HALF_WIDTH ), LCD_TOP_ROW );
    lcd.print( "cmH20" );
#endif
    lcd.setCursor( LCD_BORDER, LCD_BOTTOM_ROW );
    lcd.print( "1:" );
    lcd.print( (float)getSysValue( E_IE_RATIO )/100 );

    lcd.setCursor( ( LCD_HALF_WIDTH ) + LCD_BORDER, LCD_BOTTOM_ROW );
    lcd.println( getSysValue( E_TIDAL)  );

    lcd.setCursor( ( LCD_QUARTER_WIDTH ) + ( LCD_HALF_WIDTH ), LCD_BOTTOM_ROW );
    lcd.println( "ml" );
}
