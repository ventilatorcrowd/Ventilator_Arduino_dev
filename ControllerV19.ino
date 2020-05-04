
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

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



#include <Wire.h>
#include <TimerOne.h>                        // Interrupt library to keep the control cycle regular .. https://playground.arduino.cc/Code/Timer1/ 

#include"Controller.h"
#include"display.h"
#include"motor.h"
#include"settings.h"
#include"rotor_leds.h"
#include"alarms.h"


const bool PRODUCTIION_CODE = false;        // This is non-production code - not tested and includes calibration routines




const U32 ENTER_CALIBRATION = 5000;  // Length of time in milli-seconds that the select button has to be pushed to enter calibration mode



// updated control variables ..
// Control parameters must only change at the end of a breath cycle
// We can't have rates and values changing half-way through a breath and in an inconsistent manner
// Therefore when the control parameters are updated they are stored in this variables, then 'swapped in' at the end of a stroke
// ALSO .. we have to be careful about race-conditions for any shared variables. We can't have the interrupt driven control loop
// using a variable that has been partially updated by the code that was interrupted
// Therefore the use of a 'paramUpdateSemaphor' and a halting of interrupts to ensure that parameter updating is synchronised

volatile bool paramUpdateSemaphore;                  // TRUE -> Control parameters have been updated, reset to FALSE on use




U16 tick = 0;                             // One tick each time the controller 'main-loop' interrupt is fired

eBState breathState = E_INHALE_STATE;           // starts with machine driving breathing
eRunState runState = E_NORMAL;

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void setup( void )
{

    // Initialize serial and wait for port to open:
    Serial.begin(38400);

    while (!Serial) 
    {
      ; // wait for serial port to connect. Needed for native USB port only
    }


    // initialize the LCD
    setupDisplay();
    rotorLEDSetup();
    settingsSetup();
    alarmSetup( );
    motorSetup();
    
    DisplayVersionNum (  );

    updateDisplay();

    runState = E_NORMAL; // set to normal running mode


    digitalWrite( LED_BUILTIN, LOW );
    
    breathState = E_INHALE_STATE;

    paramUpdateSemaphore = false;               // determines access to the control parameters. Set to TRUE when the parameters have been updated
    Serial.println( "Setup Interrupt" );
    
    Timer1.initialize( TIME_BETWEEN_TICKS );                        // Set the timer interrupt
    Timer1.attachInterrupt( ventControlInterrupt, TIME_BETWEEN_TICKS );    // Call the ventilator control loop 100 times per second (probably does not need to be that fast! )

}



/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void loop()
{

    testForChangeSettings();



}

/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void ventControlInterrupt( void ) 
{

  tick = tick + 1;                               // This will be called, hence incremented every 'TIME_BETWEEN_TICKS' microseconds

  //ledFlashCount = ledFlashCount + 1;
  //if (ledFlashCount < 10) {
  //  digitalWrite(E_INHALE_LED, HIGH);
  //  digitalWrite(E_EXHALE_LED, LOW);
  //}
  //if ((ledFlashCount > 10) && (ledFlashCount < 20)) {
  //  digitalWrite(E_INHALE_LED, LOW);
  //  digitalWrite(E_EXHALE_LED, HIGH);
  //}
  //if (ledFlashCount == 21) {
  //  ledFlashCount = 0;
  //}
  
  motorControl( &tick , &breathState );


  // Second section manages the state transition
  // Depending on which state we are in (E_INHALE_STATE or E_EXHALE_STATE) .. we look at the elapsed time, and determine if it is time to change state
  // If it is time to change state, then we change the state (as stored in 'breathState')


}
