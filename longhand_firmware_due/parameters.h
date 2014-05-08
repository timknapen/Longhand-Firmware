#ifndef PARAMETERS_H
#define PARAMETERS_H

// commands to send back to the controller software
#define	MACHINE_STOPPED "_c0"

#define INVERT_X_DIR     1      // is the direction of the X axis inverted?
#define INVERT_Y_DIR     1      // is the direction of the Y axis inverted?
#define INVERT_Z_DIR     1      // http://youtu.be/EPIPLahykrI?t=1m48s



// Set to 1 if enable pins are inverting
// inverting = active low

#define INVERT_ENABLE_PINS 1

#if INVERT_ENABLE_PINS == 1
#define ENABLE_ON LOW
#else
#define ENABLE_ON HIGH
#endif



#endif




