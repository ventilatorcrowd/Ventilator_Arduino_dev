

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


#ifndef _ROTOR_H
#define _ROTOR_H

#include "Controller.h"

#define END_FUNCTION_CALL  0xFF
#define NOCHANGE           0xAA

typedef enum
{
    E_NO_LEDS,
    E_INHALE_LEDS,
    E_EXHALE_LEDS,
  
} eLedPattern;


typedef enum
{
    E_ENTER_BUT,
    E_CLEAR_BUT,
    E_BACK_BUT,
    E_MAX_BUTS,
}  eButton;


typedef enum
{
  E_UNKNOWN,
  E_FALSE,
  E_TRUE,
} eButtonConfState;

//const U8  BUT3_PRESSED  = (0x04);

void setPaternLeds_Inhale ( eLedPattern );

eButtonConfState getButtonsConfirmation( eButton button, U16 duration =0 );


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16  readRotorButton (  void );


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
void rotorLEDSetup( void );


/*---------------------------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------------------------*/
U16 getKnobIncrement(  void );

#endif
