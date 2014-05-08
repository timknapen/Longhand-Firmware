#ifndef PINS_H
#define PINS_H

/****************************************************************************************
 * Arduino DUE pin assignment
 * 
 * These are the pins for Longhand Controller PCB V1
 * They also work with the Arduino WiFiShield, when testing SD reads etc.
 *
 ****************************************************************************************/

// SC CARD (works for Longhand Controller PCB V1 and Arduino WiFiShield)
#define CSPIN 4

// X STEPPER
#define X_ENABLE_PIN    (byte)10
#define X_STEP_PIN      (byte)12
#define X_DIR_PIN       (byte)11


// Y STEPPER
#define Y_ENABLE_PIN	(byte)22
#define Y_STEP_PIN	(byte)30
#define Y_DIR_PIN	(byte)34


// Z STEPPER
#define Z_ENABLE_PIN	(byte)44
#define Z_STEP_PIN      (byte)50
#define Z_DIR_PIN	(byte)52


// MICRO STEPPING
// micro stepping is done by jumper in Longhand Controller PCB v1
// so we don't really use these anymore
//#define MS1_PIN			(byte)23
//#define MS2_PIN			(byte)24
//#define MS3_PIN			(byte)25


// END STOPS
#define X_MIN_PIN		(byte)9	       
#define Y_MIN_PIN		(byte)2
#define Z_MIN_PIN		(byte)3


#endif



