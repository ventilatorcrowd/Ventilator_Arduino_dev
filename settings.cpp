

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


#include <TimerOne.h> 
#include "Controller.h"
#include "rotor_leds.h"
#include "Display.h"
#include "motor.h"

// Physical set up constants


static bool changeSettings( void ) ;



typedef enum
{
    E_INIT,
    E_SET_VALUES,
    E_SET_PROCESS_VALUES,
    E_SET_ACCEPT_VALUES,
    E_WAIT_TO_TRANSFER,
} eChangeSetting_state;



typedef enum
{
    E_BUT_NOT_PRESSED,
    E_BUT_PRESSED,
    E_CHANGE_SETTINGS,

} eButton_State;

/* hold temp value to change all systems values simultaneously */
static U16 temp_system_data[ E_MAX_DATA_VALUES ];
    
static eButton_State buttonState = E_BUT_NOT_PRESSED;
static eChangeSetting_state state = E_INIT;


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void settingsSetup( void )
{
    buttonState = E_BUT_NOT_PRESSED;
    state = E_INIT;

}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void testForChangeSettings ( void )
{
    static U16 timeDuration;

    switch ( buttonState )
    {
    case  E_BUT_NOT_PRESSED:
        if ( readRotorButton ( ) == LOW )
        {
          Serial.println( "E_BUT_NOT_PRESSED   Low" );
            buttonState = E_BUT_PRESSED;
            timeDuration = millis();
        }
    break;
    
    case E_BUT_PRESSED:
        if ( readRotorButton ( ) == LOW )
        {
            if ( ( millis() - timeDuration) > 500  )
            {
                 buttonState = E_CHANGE_SETTINGS;
            }
        }
        else
        {
            buttonState = E_BUT_NOT_PRESSED;
        }
    break;


    case E_CHANGE_SETTINGS:
        // end when values channged and accepted at end of cycle
        if ( changeSettings() == true )
        {
            buttonState = E_BUT_NOT_PRESSED;

            // need to loop for all possible values to change ???????

        }
    break;



    default:
    break;
    }


}







/*---------------------------------------------------------------------------------
 *
 *                Modify local copies of data, copied accross when all accepted
 * 
 *--------------------------------------------------------------------------------*/
static bool modifyDateValue( E_data_type type )
{
    U16 knob_value;
    bool retValue = false;

    knob_value = getKnobIncrement( );

    if ( knob_value != END_FUNCTION_CALL )
    {
        if ( knob_value != NOCHANGE )
        {
            temp_system_data[ type ] +=  ( knob_value * getSysInc( type ) );

            if ( temp_system_data[ type ]  > getSysMax( type ) )
            {
                 temp_system_data[ type ] = getSysMax( type ); 
            }
            else if ( temp_system_data[ type ]  < getSysMin( type ) )
            {
                 temp_system_data[ type ] = getSysMin( type ); 
            }

            DisplaySetUpValue( temp_system_data[ type ] );
//          lcd.print( "  " );
 //         textDisplay( 15, 2 ,system_data[ type ].display_msg, false );

        }

    }
    else
    {
        retValue = true;
    }

    return retValue;
}




/*---------------------------------------------------------------------------------
 *
 *  when all values changed or stepped through returns true
 *  controlled by states
 *
 *--------------------------------------------------------------------------------*/
static bool changeSettings( void ) 
{
    static U16 cnt;
    bool retValue = false;


    switch ( state )
    {
    case E_INIT:
        cnt = 0;
        state = E_SET_VALUES;
    break;

    case E_SET_VALUES:
        clearDisplay();
        DisplaySetUpMsg( getSysSet_msg( (E_data_type)cnt ) );
        
        // set temp value to current values 
        temp_system_data[ cnt ]  =  getSysValue( (E_data_type)cnt );
        
        DisplaySetUpValue ( temp_system_data[ cnt ] );
        state = E_SET_PROCESS_VALUES;

    break;

    case E_SET_PROCESS_VALUES:

    
        if ( true == modifyDateValue(  (E_data_type)cnt  ) )
        {
            /* process next parameter */
            cnt++;

            
            Serial.println( "CNT inc" );

            if ( cnt >= (int)E_MAX_DATA_VALUES )
            {
                state = E_SET_ACCEPT_VALUES;

            }
            else
            {
                DisplaySetUpMsg( getSysSet_msg( (E_data_type)cnt ) );
                // set temp value to current values 
                temp_system_data[ cnt ]  =  getSysValue( (E_data_type)cnt );
        
                DisplaySetUpValue ( temp_system_data[ cnt ] );
            }
        }

    break;

    case E_SET_ACCEPT_VALUES :

        /* give change to undo all changes */
        for ( cnt = 0; cnt < E_MAX_DATA_VALUES  ; cnt++ )
        {
           setSysValue( (E_data_type)cnt, temp_system_data[ cnt ] );
        }

                // Now signal to the interrup-driven control loop that it can pick up the new control parameters
                // when it is ready to do to (at the start of a cycle)
                Timer1.stop();                   // Halt the interrupt so that there is no chance of a race condition
                paramUpdateSemaphore = true;     // Inform the main control loop that it can pick up the new control parameters
                Timer1.start();                  // Re-start the main control-loop interrupt

                clearDisplay();
                textDisplay( 0, 0, "Adjusting ...", false );
        
         
        state = E_WAIT_TO_TRANSFER;
    break;
    

    case E_WAIT_TO_TRANSFER:

         if ( paramUpdateSemaphore  == false )   // wait until this update has been picked up
         {
              clearDisplay();
              updateDisplay();
              retValue = true;
              state = E_INIT;
         }

    break;
    }

   return retValue;
}




/********************

  // We don't actualy update the control parameters directly .. this is done within      
  // Counts the number of loops that the select button has to be pushed for, before 
  // going into calibration mode the control loop at the end of a cycle
  // What we do here is to collect an updated set of parameters, and set a semaphore to 
  // indicate to the control loop to pick them up


  // Next section triggers device calibration .. but only for non-production (test code).
  if (not PRODUCTIION_CODE) 
  {
    // Counts the number of loops that the select button has to be pushed for, before going into calibration mode
    U32 timeSelected = millis(); 
    U16 selectButtonData = digitalRead( SELECT_BUTTON );

    if ( selectButtonData == LOW ) 
    {
      textDisplay( 0, 0, "Keep button pushed");
      textDisplay( 0, 1, "to calibrate" );
    }
    while ( ( selectButtonData == LOW ) && ( ( millis() - timeSelected ) < ENTER_CALIBRATION ) ) 
    {
      selectButtonData = digitalRead( SELECT_BUTTON );
    }

    if ( ( selectButtonData == LOW ) && ( ( millis() - timeSelected ) >= ENTER_CALIBRATION ) ) 
    {
      calibrate();
    }
  }






  newRespRate = int( getKnob( RESP_RATE_MIN, RESP_RATE_MAX, respRate, RESP_RATE_STEP, "bpm" ) );
bool upDateValue( float settingMin, float settingMax, float *currentVal, float setStep, const char * units_str )




  // Serial.println("Inspiratory Pressure");               // Not implemented in this version
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Inspiratory Press:");
  // newInspPressure = int(getKnob(INSP_PRESS_MIN, INSP_PRESS_MAX, inspPressure, 1.0, "cmH20"));

  // Serial.println("I.E. Ratio");
  clearDisplay();
  textDisplay( 0, 0, "I.E. Ratio:" );

  newIERatio = getKnob( I_E_RATIO_MIN, I_E_RATIO_MAX, iERatio, I_E_RATIO_STEP, "" );

  // Serial.println("TIDAL");
  clearDisplay();
  textDisplay( 0, 0, "TIDAL:");

  newTidal = int( getKnob(TIDAL_MIN, TIDAL_MAX, tidal, TIDAL_STEP, "ml" ));

  // Now signal to the interrup-driven control loop that it can pick up the new control parameters 
  // when it is ready to do to (at the start of a cycle)
  Timer1.stop();                   // Halt the interrupt so that there is no chance of a race condition
  paramUpdateSemaphore = true;     // Inform the main control loop that it can pick up the new control parameters
  Timer1.start();                  // Re-start the main control-loop interrupt

  clearDisplay();
  textDisplay( 0, 0, "Adjusting ...");

  while ( paramUpdateSemaphore ) {};  // wait until this update has been picked up

  // Finished sending updated parameters to control loop


  clearDisplay();
  updateDisplay();

  calcTicksPerCycle( respRate, iERatio );           // Update the breath cycles based on the new settings of RR and I.E. ratio



  ******************************************/
