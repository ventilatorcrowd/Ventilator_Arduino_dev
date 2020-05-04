

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


#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#define VERSION_NUM_MAJOR    0
#define VERSION_NUM_MINOR    19

typedef   unsigned char  U8;
typedef   unsigned int   U16;
typedef   unsigned long  U32;
typedef   char           S8;
typedef   int            S16;
typedef   long           S32;

const U32 TIME_BETWEEN_TICKS = 10000;                   // Time between the main-control interrupt being called in microseconds

typedef enum
{
    E_INHALE_STATE,
    E_EXHALE_STATE,
} eBState;


typedef enum
{
    E_NORMAL,
    E_ADJUST_RR,
} eRunState;

extern volatile bool paramUpdateSemaphore;  

#endif
