#ifndef PARAMETERS_H
#define PARAMETERS_H

// commands to send back to the controller software
#define	MACHINE_STOPPED "_c0"

#define INVERT_X_DIR     1
#define INVERT_Y_DIR     1
#define INVERT_Z_DIR     1



// Set to one if enable pins are inverting
// inverting = active low

#define INVERT_ENABLE_PINS 1

#if INVERT_ENABLE_PINS == 1
#define ENABLE_ON LOW
#else
#define ENABLE_ON HIGH
#endif



#endif




