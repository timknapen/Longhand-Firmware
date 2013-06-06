// Longhand Firmware

#ifndef PINS_H
#define PINS_H

/****************************************************************************************
 * Arduino DUE pin assignment
 * 
 ****************************************************************************************/
 

#define CSPIN 4

// POWER PINS
// to power the stepper drivers we take the power from two high current pins on the Due
// a high current pin on Arduino Due can source 15mA and our stepper drivers take max 8mA

// X STEPPER
#define X_ENABLE_PIN	(byte)22
#define X_STEP_PIN		(byte)23
#define X_POWER_PIN		(byte)24
#define X_DIR_PIN		(byte)25


// Y STEPPER
#define Y_ENABLE_PIN	(byte)26
#define Y_STEP_PIN		(byte)27
#define Y_POWER_PIN		(byte)28
#define Y_DIR_PIN		(byte)29


// Z STEPPER
#define Z_ENABLE_PIN	(byte)30
#define Z_STEP_PIN		(byte)31
#define Z_POWER_PIN		(byte)32
#define Z_DIR_PIN		(byte)33


// MICRO STEPPING
#define MS1_PIN			(byte)34
#define MS2_PIN			(byte)35
#define MS3_PIN			(byte)36	


// END STOPS
#define X_MIN_PIN		(byte)37
#define Y_MIN_PIN		(byte)38	
#define Z_MIN_PIN		(byte)39


#endif


