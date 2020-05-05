#ifndef IO_CONFIG_H
#define IO_CONFIG_H
 #ifdef UNO_CONFIG
//   const int POWER_ON_OFF      = x;  // System Power Button (Interrupt 4) **** I/O TBD ****
   const int INHALE_LED          = 5;  // Green LED - ( LEDG_PIN_VIN on circuito.io )
   const int EXHALE_LED          = 6;  // Blue LED  - ( LEDR_PIN_VIN on circuito.io )
   const int ROTARY_CLK          = 2;  // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
   const int ROTARY_DT           = 3;  // Connected to DT  on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
   const int SELECT_BUTTON       = 4;  // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )
   const int SERVO_PIN           = 7;
   const int PWM_PIN             = 9;  // Pin for the PWM output
   const int WATCHDOG            = 10; // Pin for the hardware watchdog
   const int PRESSURE_SENSOR_PIN = A0; // Pin for the pressure sensor
 #else
 #ifdef MEGA_CONFIG
   const int POWER_ON_OFF        = 19; // System Power Button (Interrupt 4)
   const int INHALE_LED          = 21; // Green LED - ( LEDG_PIN_VIN on circuito.io )
   const int EXHALE_LED          = 20; // Blue LED  - ( LEDR_PIN_VIN on circuito.io )
   const int ROTARY_CLK          = 24; // Connected to CLK on KY-040 ( ROTARYENCI_PIN_CLK on circuito.io )
   const int ROTARY_DT           = 23; // Connected to DT  on KY-040 ( ROTARYENCI_PIN_D on circuito.io )
   const int SELECT_BUTTON       = 22; // Push switch built into the rotary encoder ( ROTARYENCI_PIN_S1 on circuito.io )
   const int SERVO_PIN           = 25;
   const int PWM_PIN             = 26; // Pin for the PWM output
   const int WATCHDOG            = 27; // Pin for the hardware watchdog
   const int PRESSURE_SENSOR_PIN = A8; // Pin for the pressure sensor
 #endif
 #endif
#endif
