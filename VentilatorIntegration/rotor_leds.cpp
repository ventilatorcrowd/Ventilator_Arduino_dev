
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include "Display.h"
#include "motor.h"
#include "rotor_leds.h"



typedef enum
{
    BUT_NO_REQUEST,
    BUT_WAIT_FOR_PRESS,
    BUT_WAIT_FOR_RELEASE,
    BUT_WAIT_FOR_TIMED_RELEASE,
} eButtonState;



typedef enum
{
    E_ROT_UNKNOWN,
    E_ROT_NOT_PRESSED,
    E_ROT_READING_VALUE,
} eRotor_state;


// Physical set up constants
const U16 INHALE_LED    = 25;             // GReen LED - ( LEDG_PIN_VIN on circuito.io )
const U16 EXHALE_LED    = 27;             // Red LED - ( LEDR_PIN_VIN on circuito.io )

const U16 ROTARY_CLK    = 31;             // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
const U16 ROTARY_DT     = 33;              // Connected to DT on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
const U16 SELECT_BUTTON = 35;          // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )

/* status of button press for each button */
eButtonState button_state[ E_MAX_BUTS ] = { BUT_NO_REQUEST, BUT_NO_REQUEST, BUT_NO_REQUEST };

const U16 buttonID[ E_MAX_BUTS ] = { 53, 51, 49 };


    
static U16 pinALast;

static eLedPattern  led_pattern_state; 
/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 readRotorButton (  void )
{
    return  digitalRead(SELECT_BUTTON);
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void rotorLEDSetup( void )
{
   U8 loopCnt = 0;
   
    pinMode( INHALE_LED, OUTPUT );            // These are the red and green LEDS that indicate the breathing state
    pinMode( EXHALE_LED, OUTPUT );
    digitalWrite( INHALE_LED, HIGH );
    digitalWrite( EXHALE_LED, HIGH );


    pinMode( SELECT_BUTTON, INPUT );        // input from centre button of the rotary encoder
    pinMode ( ROTARY_CLK, INPUT );               // Input from CLK of rotary encoder
    pinMode ( ROTARY_DT, INPUT );               // Input from DT of rotary encoder
    pinALast = digitalRead( ROTARY_CLK );        // Remember the state of the rotary encoder CLK

    digitalWrite( LED_BUILTIN, LOW );
    
    // initialize the pushbutton pin as an input:
    pinMode( buttonID[ E_ENTER_BUT ], INPUT );
    pinMode( buttonID[ E_CLEAR_BUT ], INPUT );
    pinMode( buttonID[ E_BACK_BUT ], INPUT ); 

    /* nitioalse buttons states */
    for ( loopCnt = 0; loopCnt < (U8)E_MAX_BUTS; loopCnt++ )
    {
        button_state[ E_MAX_BUTS ] = BUT_NO_REQUEST;
    }
      
    led_pattern_state = E_NO_LEDS;
}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
bool isButtonsPressed ( eButton button )
{
    bool retButtonState = false;
    U8 buttonState = 0;
    
    // read the state of the pushbutton 1 value:
    buttonState = digitalRead( buttonID[ button ] );

    // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if ( buttonState == HIGH ) 
    {
Serial.println( "buttonState == HIGH" );

        retButtonState = true;
    } 
 
    return retButtonState;
}

/*---------------------------------------------------------------------------------
 *
 *  Funtion to return true if a button is pressed and released, otherwise false
 *  If a time is provided when released returns true if pressed fpr that time in 
 *  milli seconds.
 *
 *--------------------------------------------------------------------------------*/
eButtonConfState getButtonsConfirmation( eButton button, U16 duration )
{
    static U16 startTime[ E_MAX_BUTS ] ;
//    static U8 releaseCnt[ E_MAX_BUTS ];
    static U16 pressTime[ E_MAX_BUTS ];
    
    /* only one return value as single threaded and this function is not re entrant */
    eButtonConfState returnState = E_UNKNOWN ;
     
    switch ( button_state[ button ] )
    {
    case  BUT_NO_REQUEST:
        
        pressTime[ button ] = duration;
        startTime [ button ] = 0;
        
        if ( isButtonsPressed( button ) == true )
        {
             button_state[ button ] = BUT_WAIT_FOR_PRESS;
        }
        
 //   Serial.println( "BUT_NO_REQUEST" );
        break;
       
    case BUT_WAIT_FOR_PRESS:

            if ( pressTime[ button ] > 0 )
            {
                startTime[ button ] = millis();
                button_state[ button ] = BUT_WAIT_FOR_TIMED_RELEASE;
    Serial.print( pressTime[ button ]);
     Serial.println( "   BUT_WAIT_FOR_TIMED_RELEASE" );
            }
            else
            {
                button_state[ button ] = BUT_WAIT_FOR_RELEASE;
    Serial.println( "BUT_WAIT_FOR_RELEASE" );
            }
 
        break;
      
    case BUT_WAIT_FOR_RELEASE:
        if ( isButtonsPressed( button ) == false )
        {
            button_state[ button ] = BUT_NO_REQUEST;
            returnState = E_TRUE;
             Serial.println( "RELEASED" );
        }
        break;
      
    case BUT_WAIT_FOR_TIMED_RELEASE:
        if ( isButtonsPressed( button ) == false )
        {
            /* test if time has passed */
            U16 timePassed = millis( ) - startTime[ button ];
            if (  timePassed  >= pressTime[ button ] )
            {  
                returnState = E_TRUE;
                Serial.print ( timePassed );
                Serial.println( "   TIMED RELEASE pass" );
            }
            else
            {
                returnState = E_FALSE;
            }

            button_state[ button ] = BUT_NO_REQUEST;
        }
        break;
    }

    return returnState;
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void setPaternLeds_Inhale ( eLedPattern  pattern )
{
    if ( led_pattern_state != pattern )
    {
        switch (  pattern   )
        {

          case E_NO_LEDS:
              digitalWrite( INHALE_LED, HIGH );
              digitalWrite( EXHALE_LED, HIGH );
           break;
           
          case E_INHALE_LEDS:
              digitalWrite( INHALE_LED, HIGH );
              digitalWrite( EXHALE_LED, LOW );
           break;

          case E_EXHALE_LEDS:
             digitalWrite( INHALE_LED, LOW );
             digitalWrite( EXHALE_LED, HIGH );             
          break;
        }
        
        led_pattern_state = pattern;        
    }
}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getKnobIncrement(  void )
{
    static eRotor_state RState = E_ROT_UNKNOWN;
    static U16 aVal;
    static boolean bCW;
    U16 retValue = NOCHANGE;


    eButtonConfState buttonData = getButtonsConfirmation( E_ENTER_BUT , 0 );

    /*  test for rotary changes and end when the button pressed */
    switch ( RState )
    {
    case E_ROT_UNKNOWN:
    case E_ROT_NOT_PRESSED:
        aVal = 0;
        bCW = false;

        RState = E_ROT_READING_VALUE;
    
    break;


    case E_ROT_READING_VALUE:

        /*  Test for button pressed */
        if (  buttonData != E_TRUE ) 
        {
            aVal = digitalRead( ROTARY_CLK );

            if ( aVal != pinALast )  // Means the knob is rotating
            {
                // if the knob is rotating, we need to determine direction
                // We do that by reading pin B.
                if ( digitalRead( ROTARY_DT ) != aVal ) 
                {  
                    retValue = 1;
                }
                else
                {
                    retValue = -1;
                }
                pinALast = aVal;
            }
            else
            {
                retValue = NOCHANGE;
            }
        }
        else
        {
            RState = E_ROT_NOT_PRESSED;
            retValue = END_FUNCTION_CALL;
            Serial.println( "END_FUNCTION_CALL" );
        }	

    break;

    default:
        /* Error should never get here */
        Serial.println( "getKnobIncrement Error" );
    break;

    }

    return ( retValue );
}
