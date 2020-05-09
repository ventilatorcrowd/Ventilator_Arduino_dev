
/*----------------------------------------------------------------------------\
|          Ventilator Crowd Open Source Software                              |
|                                                                             |
|-----------------------------------------------------------------------------|
|                                                                             |
|   Module Name  : Settings                                                    |
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
|   Generic Setting functions                                             |
|                                                                             |
\----------------------------------------------------------------------------*/


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
    E_WAIT_FOR_PRESSED,
    E_CHANGE_SETTINGS,

} ePress_State;

/* hold temp value to change all systems values simultaneously */
static U16 temp_system_data[ E_MAX_DATA_VALUES ];
    
static ePress_State pressState = E_WAIT_FOR_PRESSED;
static eChangeSetting_state state = E_INIT;

#define CHANGE_DELAY_TIME  1000      // 1 second delay to change values

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void settingsSetup( void )
{
    pressState = E_WAIT_FOR_PRESSED;
    state = E_INIT;

}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void testForChangeSettings ( void )
{

   eButtonConfState bState;

    switch ( pressState )
    {
    case  E_WAIT_FOR_PRESSED:
        
        bState = getButtonsConfirmation( E_ENTER_BUT, CHANGE_DELAY_TIME );
        
        if ( bState  == E_TRUE )
        {
          Serial.println( "BUT 1  pressed" );
            pressState = E_CHANGE_SETTINGS;
        }
    break;
    

    case E_CHANGE_SETTINGS:
        // end when values channged and accepted at end of cycle
        if ( changeSettings() == true )
        {
            pressState = E_WAIT_FOR_PRESSED;

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

    /* 
     * test for clear button and exit with out change 
     */
    if ( E_TRUE == getButtonsConfirmation( E_CLEAR_BUT, 0 ) )
    {
         clearDisplay();
         updateDisplay();
         retValue = true;
         state = E_INIT;      
    }

    
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
        for ( cnt = 0; cnt < E_MAX_DATA_VALUES; cnt++ )
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
