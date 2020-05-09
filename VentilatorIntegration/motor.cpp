/*----------------------------------------------------------------------------\
|          Ventilator Crowd Open Source Software                              |
|                                                                             |
|-----------------------------------------------------------------------------|
|                                                                             |
|   Module Name  : Motor Control                                              |
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
|   Generic Motor set up and run functions                                    |
|                                                                             |
\----------------------------------------------------------------------------*/

#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include "Controller.h"
#include "rotor_leds.h"
#include "Display.h"
#include "motor.h"


#ifdef SERVO_MOTOR
/*################################################################################
 *   
 *                                SERVO MOTOR
 *
 *###############################################################################*/
#include "Servo.h"

// Define shape of driver waveform
static const U16 inhale_drive[] = { 750, 761, 772, 783, 795, 806, 817, 828, 840, 851, 862, 873, 885, 896, 
                             907, 918, 930, 941, 952, 963, 975, 986, 997, 1008, 1020, 1031, 1042, 
                             1053, 1065, 1076, 1087, 1098, 1110, 1121, 1132, 1143, 1155, 1166, 1177, 
                             1188, 1200, 1220, 1240, 1260, 1280, 1300, 1320, 1340, 1360, 1380, 1400, 
                             1420, 1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580, 1600, 1620, 1640, 
                             1660, 1680, 1700, 1720, 1740, 1760, 1780, 1800, 1820, 1840, 1860, 1880, 
                             1900, 1920, 1940, 1960, 1980, 2000, 2012, 2025, 2037, 2050, 2062, 2075, 
                             2087, 2100, 2112, 2125, 2137, 2150, 2162, 2175, 2187, 2200, 2212, 2225, 
                             2237, 2250};

Servo  pumpServo;                // Create the servo object

static const U16 DRIVE_VAL_MIN = 750;                 // minimum value for the output drive (position during exhale)

//
// Motor can be Stepper or servo motor
//
const U16 MOTOR_PIN     = 43;

const U16 RAW_ACTUATOR_MIN =   700; // minimum, unscalled value direct to actuatory
const U16 RAW_ACTUATOR_MAX =  2400; // max
const U16 RAW_ACTUATOR_STEP =   50;


#else
#include <TimerOne.h> 
/*################################################################################
 *   
 *                                PWM MOTOR
 *   
 *###############################################################################*/
/* Define shape of driver waveform */
static const U16 inhale_drive[] = {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,
	                           90,95,100,105,110,115,120,125,130,135,140,145,150,
                                   155,160,165,170,175,180,185,190,195,200,217,235,252,
                                   270,287,305,322,340,357,375,392,410,427,445,462,480,497,
                                   515,532,550,567,585,602,620,637,655,672,690,707,725,742,
                                   760,777,795,812,830,847,865,882,900,906,912,918,924,930,936,
                                   943,949,955,961,967,973,979,986,992,998,1004,1010,1016,1023};

static const U16 DRIVE_VAL_MIN = 0;                 // minimum value for the output drive (position during exhale)

const U16 MOTOR_PIN     = 43;

const int RAW_ACTUATOR_MIN = 0;  // minimum, unscalled value direct to actuatory
const int RAW_ACTUATOR_MAX = 1023; // max
const int RAW_ACTUATOR_STEP = 20;

#endif


static const U16 DRIVE_TABLE_SIZE = sizeof( inhale_drive )/sizeof( U16 );            // length of the INHALE_DRIVE table


// define control constants for the ventilator
static const U16 INSP_PRESS_MAX = 20;           // Inspiratory pressure
static const U16 INSP_PRESS_MIN = 5;
static const U16 INSP_PRESS_DEFAULT = 15;
static const U16 INSP_PRESS_STEP = 1;

static const U16 RESP_RATE_MAX = 30;             // Respratory rate
static const U16 RESP_RATE_MIN = 10;
static const U16 RESP_RATE_DEFAULT = 20;
static const U16 RESP_RATE_STEP = 1;

const U16 TIDAL_MAX = 300;                   // TIDAL
const U16 TIDAL_MIN = 200;
const U16 TIDAL_DEFAULT = 250;
const U16 TIDAL_STEP = 10;

/* IE values * 100 to avoid floats */
/* converted to float when needed  at display etc */
const U16 I_E_RATIO_MAX = 300;           // Inspiratory - expiratory ratio
const U16 I_E_RATIO_MIN = 20;
const U16 I_E_RATIO_DEFAULT = 100;
const U16 I_E_RATIO_STEP = 20;

S8  INSP_MSG[] = {  "Ins press" };
S8  INSP_SET_MSG[] = { "Inspiratory Pressure" };
S8  TIDAL_MSG[] = {  "Tidal" };
S8  TIDAL_SET_MSG[] = {  "Tidal" };

S8  RESP_MSG[] = {   " resp" };
S8  RESP_SET_MSG[] = { "Respiratory Rate" };

S8  IER_MSG[] = {      "IERatio" };
S8  IER_SET_MSG[] = {   "I.E. Ratio" };


/* 
 * control variables
 * system data all in one array 
 */
static S_data_value system_data[ E_MAX_DATA_VALUES ] =  
                {   { E_INSP_PRESS, INSP_PRESS_MIN, INSP_PRESS_MAX, INSP_PRESS_DEFAULT, INSP_PRESS_STEP, INSP_MSG, INSP_SET_MSG },
                    { E_RESP_RATE, RESP_RATE_MIN, RESP_RATE_MAX, RESP_RATE_DEFAULT, RESP_RATE_STEP, RESP_MSG, RESP_SET_MSG },
                    { E_TIDAL, TIDAL_MIN, TIDAL_MAX, TIDAL_DEFAULT, TIDAL_STEP, TIDAL_MSG, TIDAL_SET_MSG },
                    { E_IE_RATIO, I_E_RATIO_MIN, I_E_RATIO_MAX, I_E_RATIO_DEFAULT, I_E_RATIO_STEP, IER_MSG, IER_SET_MSG } };



const U32 ENTER_CALIBRATION = 5000;  // Length of time in milli-seconds that the select button has to be pushed to enter calibration mode


// updated control variables ..
// Control parameters must only change at the end of a breath cycle
// We can't have rates and values changing half-way through a breath and in an inconsistent manner
// Therefore when the control parameters are updated they are stored in this variables, then 'swapped in' at the end of a stroke
// ALSO .. we have to be careful about race-conditions for any shared variables. We can't have the interrupt driven control loop
// using a variable that has been partially updated by the code that was interrupted
// Therefore the use of a 'paramUpdateSemaphor' and a halting of interrupts to ensure that parameter updating is synchronised

U16 inspPressure =  INSP_PRESS_DEFAULT;
U16 respRate = INSP_PRESS_DEFAULT;
U16 tidal = TIDAL_DEFAULT;
U16 iERatio = I_E_RATIO_DEFAULT;


const U32 TICKS_PER_MINUTE = 60000000 / TIME_BETWEEN_TICKS;       // assume milli-second clock tick .. calibrate to clock speed



static U16 ticksPerInhale = 0;                    // Number of clock-ticks per inhale
static U16 ticksPerExhale = 0;                    // Nmber of clock-ticks per exhale
String outPutLog = "";

static void calcTicksPerCycle( void ) ;

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getSysValue ( E_data_type type )
{
    return ( system_data[ type ].value );
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getSysInc ( E_data_type type )
{
    return ( system_data[ type ].set_value );
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getSysMin ( E_data_type type )
{
    return ( system_data[ type ].rate_min );
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getSysMax ( E_data_type type )
{
    return ( system_data[ type ].rate_max );
}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void setSysValue ( E_data_type type , U16 newValue )
{
     system_data[ type ].value = newValue;
}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
S8 * getSysSet_msg ( E_data_type type )
{
    return system_data[ type ].setting_msg;
}





/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void motorSetup( void )
{

    
#ifdef SERVO_MOTOR
    pumpServo.attach( MOTOR_PIN );
    pumpServo.write( ( (U32)DRIVE_VAL_MIN * tidal )/ TIDAL_MAX ) ;  // By default -set the controller to 'fully open'
#else
    Timer1.pwm( MOTOR_PIN, DRIVE_VAL_MIN); // By default -set the controller to 'fully open'
#endif

    calcTicksPerCycle( );     // Update breath tick rates based on default RR and I.E.

    Serial.println( "Motor to squeeze BVM" );

}


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void  motorControl( U16 *tick , eBState *breathState ) 
{
    U16 positionInDriveTable = 0;             // the offset down the output drive table for current clock tick
    U16 driveValue = 0;                       // The value output to the actuator
    U16 unscaledDriveValue = 0;               // Drive value before being scalled by tidal volume


    // First section outputs the correct drive value to the actuator
    // This depends on if we are inhaling (breathState == E_INHALE_STATE) or exhaling (breathState = EXHALE_STATE)
    if ( *breathState == E_INHALE_STATE ) 
    {
        // How far down the drive table for drive value at this time (tick)?
        positionInDriveTable = U16( ( (U32) DRIVE_TABLE_SIZE * (U32) *tick ) / ticksPerInhale );

        if ( positionInDriveTable > DRIVE_TABLE_SIZE ) 
        {
            positionInDriveTable = DRIVE_TABLE_SIZE;
            //Serial.println( "Size drive table Error " );
        }
        unscaledDriveValue = inhale_drive[ positionInDriveTable ];

        // Scales the drive value as a proportion of the maximum allowed tidal volume
        driveValue =  ( (U32)unscaledDriveValue * tidal ) / TIDAL_MAX;  

  //      setPaternLeds_Inhale ( E_INHALE_LEDS );

    } 
    else 
    {
        // by inference, we must be in the exhale state
        // positionInDriveTable = 0;
        // No control over the exhale control waveform - patient air vents via a valve to external air
        driveValue = ( (U32)DRIVE_VAL_MIN * tidal )/ TIDAL_MAX;
        
   //     setPaternLeds_Inhale ( E_EXHALE_LEDS );
    //     Serial.println( driveValue );

    }
#ifdef SERVO_MOTOR
     pumpServo.write( driveValue );
#else
     Timer1.setPwmDuty(MOTOR_PIN, driveValue);     // PWM Output converted to 4-20mA control signal externally
#endif
     
   //     Serial.println( driveValue );
       
    // Second section manages the state transition
    // Depending on which state we are in (INHALE_STATE or EXHALE_STATE) .. we look at the elapsed time, 
    // and determine if it is time to change state
    // If it is time to change state, then we change the state (as stored in '*breathState')

    if ( ( *tick >= ticksPerInhale ) && ( *breathState == E_INHALE_STATE ) )  
    {
        // Time to exhale
  //      Serial.println( "ON" );
        setPaternLeds_Inhale ( E_EXHALE_LEDS );
        *breathState = E_EXHALE_STATE;        // switch to exhaling
   //     pumpServo.write( 2250 );
        *tick = 0;
      }

      if ( ( *tick >= ticksPerExhale ) && ( *breathState == E_EXHALE_STATE ) )  
      {
          // Time to Inhale
   //       Serial.println( "OFF" );
          *breathState = E_INHALE_STATE;     // switch to inhaling
          setPaternLeds_Inhale ( E_INHALE_LEDS );
          // pumpServo.write( 750 );      // just for debugging pump rate
          *tick = 0;

          // At the end of each cycle, check to see if control parameters have been updated
          if ( paramUpdateSemaphore == true )     // Parameters have been updated, we need to use these on the next cycle
          {
              Serial.println( "New Data" );
              inspPressure =  (system_data[ E_INSP_PRESS ].value);
              respRate = (system_data[ E_RESP_RATE ].value);
              tidal = (system_data[ E_TIDAL ].value);
              iERatio = (system_data[ E_IE_RATIO ].value);
              calcTicksPerCycle( );
              paramUpdateSemaphore = false;         // Signal that we have finished updating the control parameters
          }
      }

}




/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
static void calcTicksPerCycle( void ) 
{
    // Calculates the number of clock cycles per inhale, exhale

    U32 ticksPerBreath = (U32)TICKS_PER_MINUTE /  respRate ;
    U32 iPlusE = 1 + ( iERatio/100);

       
    Serial.print("Ticks per breath = ");
    Serial.println(ticksPerBreath);
    
    ticksPerInhale = ( ticksPerBreath / iPlusE );
    ticksPerExhale = ( (U32)ticksPerBreath * iERatio )/ ( iPlusE * 100 );
    
    /* compenstate for iERattion */
    Serial.print("Ticks per exhale = ");
    Serial.println(ticksPerExhale);
    

    Serial.print("Ticks per inhale = ");
    Serial.println(ticksPerInhale);


}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/

void calibrate( void ) 
{
//    Timer1.stop();                   // Halt the main control interrupt - we are now in calibration mode

    U16 rawActuatorSetting  = RAW_ACTUATOR_MIN;             // set the actuator to its minumum value
    U16 lastRawActSetting = RAW_ACTUATOR_MIN;
    U16 setting = RAW_ACTUATOR_MIN;

    clearDisplay();
    /* 
    lcd.fillRect( 0, LCD_HALF_HEIGHT, LCD_WIDTH, 5, WHITE );
    lcd.fillRect( LCD_HALF_WIDTH, 0, 5, LCD_HEIGHT, WHITE );

    lcd.setTextColor( WHITE );

    while (1)                         // lock-up .. no exit from here without hard-reset!
    {
        textDisplay( 0, 1, "Raw actuator value:", false  );

        rawActuatorSetting = int( getKnob( RAW_ACTUATOR_MIN, RAW_ACTUATOR_MAX, lastRawActSetting, RAW_ACTUATOR_STEP, "(raw)" ) );

        if ( rawActuatorSetting > lastRawActSetting ) 
        {
            for (setting = lastRawActSetting; setting <= rawActuatorSetting; setting++) 
            {
                pumpServo.write( setting );
                delay(5);                                       // slow things down to remove abrupt changes
            }
        } 
        else 
        {
            for ( setting = lastRawActSetting;  setting >= rawActuatorSetting; setting-- ) 
            {
                pumpServo.write( setting );
                delay(5);                                       // slow things down to remove abrupt changes
            }
        }
        lastRawActSetting = rawActuatorSetting;
    }
    */
}
