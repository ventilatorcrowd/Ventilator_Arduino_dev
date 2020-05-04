
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include "Display.h"
#include "motor.h"
#include "rotor_leds.h"


/* Author      : Rob Collins
   Version     : 15
   Description : Simple ventilator controller providing a simple human interface for Respiratory Rate, I.E. Ratio and Tidal volume
                 Ventilator control cycle runs from a timer interrupt .. enabling control parameters to be changed whilst the controler
                 continues to provide control signals
   Changes     : V15 : Introduces intermediate variables and a 'semaphor' between the parameter change code and the main control loop
                       This is to enable changed parameters to only take effect at the end of a breathing cycle.
                       Also avoid the problem of a race condition in the case where parameter variables are partially update when a control-loop interrupt occures
                       This version uses a 'semaphor' to signal to the main control loop that parameters have been updated. This semaphor is only set whilst the
                       interrupt timers are halted

               : V16 : This version has outpur pins re-assigned to match the circuit layout defined using circutio.io
               : V17 : Added servo control to simulate output to an actuator .. needed to use ServoTimer2.h as there is a conflict between TimerOne.h and Servo.h
               : V18 : Throw away version
               : V19 : Added a 'calibration' mode - when the mode switch is held down for an extended period - enters a calibration mode .. must be removed from deployed versions!
   Circuit     : https://www.circuito.io/app?components=512,9590,9591,11021,217614,417987
               : Also add a 10k pull-up resistor between Rotary centre switch (Middle pin, 'SELECT_BUTTON' ) and +5V .. this device has no built in pull-up for centre switch
   Status      : Untested
*/




typedef enum
{
    E_ROT_UNKNOWN,
    E_ROT_NOT_PRESSED,
    E_ROT_READING_VALUE,
    E_ROT_LOCK_VALUE,
} eRotor_state;


// Physical set up constants
const U16 INHALE_LED    = 25;             // GReen LED - ( LEDG_PIN_VIN on circuito.io )
const U16 EXHALE_LED    = 27;             // Red LED - ( LEDR_PIN_VIN on circuito.io )

const U16 ROTARY_CLK    = 31;             // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
const U16 ROTARY_DT     = 33;              // Connected to DT on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
const U16 SELECT_BUTTON = 35;          // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )

const int button1Pin = 53;     // the number of the pushbutton pin
const int button2Pin = 51;     // the number of the pushbutton pin

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
    pinMode( button1Pin, INPUT );
    pinMode( button2Pin, INPUT );
    
    led_pattern_state = E_NO_LEDS;
}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U8 getButtonsPressed ( void )
{
    U8 retButtonState = 0;
    U8 buttonState = 0;
    
    // read the state of the pushbutton 1 value:
    buttonState = digitalRead( button1Pin );

    // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if ( buttonState == HIGH ) 
    {
        retButtonState = BUT1_PRESSED;
    } 

    // read the state of the pushbutton 1 value:
    buttonState = digitalRead( button2Pin );

    // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if ( buttonState == HIGH ) 
    {
        retButtonState |= BUT2_PRESSED;
    } 
 
    return retButtonState;
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
    U16 retValue = 0;

    U16 selectButtonData = digitalRead( SELECT_BUTTON );


    switch ( RState )
    {
    case E_ROT_UNKNOWN:
    case E_ROT_NOT_PRESSED:
        aVal = 0;
        bCW = false;

        RState = E_ROT_READING_VALUE;
    
    break;


    case E_ROT_READING_VALUE:

        if ( selectButtonData == HIGH ) 
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
            RState = E_ROT_LOCK_VALUE;
        }	

    break;

    case E_ROT_LOCK_VALUE:
        if ( selectButtonData == HIGH ) 
        {
            RState = E_ROT_NOT_PRESSED;
            retValue = END_FUNCTION_CALL;
       }
       else
       {
           retValue = NOCHANGE;
       }
    break;

    }

    return ( retValue );
}
