#include <SPI.h>
#include <SD.h>
#include "parameters.h"
#include "pins.h"
#include "LongPoint.h"
#include "Arduino.h"


/*------------------------------------------------------------
 
 LONGHAND DRAWING MACHINE firmware
 
 last update 07/04/2016
 by Tim Knapen
 http://www.longhand.cc/
 
 This is the firmware for
 + Arduino Due
 + Longhand PCB V1
 + 4 Pololu A4988 stepper motor driver carriers
 
 Compiled and tested with
 - Arduino 1.5.2
 - Arduino 1.5.6-r2
 - Arduino 1.5.6-r2 serialUSBPatched
  updated CDC.cpp and USBCore.cpp see: http://forum.arduino.cc/index.php/topic,140543.0.html
 - Arduino 1.5.8 ? (has both files already patched!)
 
 ------------------------------------------------------------*/

#define VERSION "V2.8.1"

// This is meant for Arduino DUE!
#ifndef _VARIANT_ARDUINO_DUE_X_
#error Please compile for Arduino Due!
#endif

// SERIAL
#define bufferLength 64                     // serial buffer length
char serialBuffer[bufferLength];            // serial buffer
int iSerialBuf = 0;							// position in the serialBuffer
#define endline '\n'						// a command always ends with a newline '\n'

// fix for mixing up of serial messages!
#define serialDelay 200
#define print(a) SerialUSB.print(a); delayMicroseconds(serialDelay);
#define println(a) SerialUSB.println(a); delayMicroseconds(serialDelay);


// BEZIER
float bezierResolution = 10.0f;				// in how many straight lines will I approximate a bezier curve?

// STATES
#define WORKING 1							// listening to serial, taking commands
#define FILEWRITE 2							// not taking commands, moving everything I receive from serial onto a file.
int state = WORKING;

// DEBUG
int debug = 0;                              // set the debug level
bool bPreview = true;						// to check if I'm doing a preview, not actually drawing.

// SPEEDS
int current_delay = 1000;					// in microsecs
int min_delay =		1000;					// the fastest speed possible
int max_delay =		3000;					// the slowest speed possible.
int acceleration =	10;					// gets added each step to the delay to calculate the acceleration speed
int microSteps =	16;					// the type of microsteps we are taking, default is 1/8th step


// DISTANCE (keep track of how long a print will take)
unsigned long travelDistance = 0L;

// POSITIONS (for steppers)
LongPoint current_pos;						// current position in steps
LongPoint target_pos;						// target position in steps
LongPoint delta_steps;						// the distances on each axis
LongPoint offSet;

float mmToStep = 85.571;		// 16microsteps
//42.929; //42.735;			// convert mm commands to steps!

// scale existing drawings, but only when drawing from file!
bool isDrawingFromFile = false;             // are we drawing from a file?
float scale = 1;                              // scale factor
int rotation = 0;                           // in 90° : 1 = 90, 2 = 180, 3 = -90

// tool selection
#define TOOL_PEN 1							// pen releases the pen and lets it fall by gravity while drawing lines.
#define TOOL_BRUSH 2						// brush holds the pen on 0, doesn't let it fall.
#define TOOL_KNIFE 3						// knife makes the pen do extra moves to rotate the knife in the correct direction before cutting
#define TOOL_LIGHT 4						// light keeps the pen in the air and can go below z=0

int tool = TOOL_PEN;
FloatPoint knifeDir = {1,0};
FloatPoint knifePos = {0,0};
float knifeRadius = 5;


//------------------------------------------------------------
void setup() {
	while (!SerialUSB);     // wait for the serialUSB to come up
	delay(100);
	print("Longhand Drawing Machine ");
	print(VERSION);
	println(" awaiting commands");
	println("Send me \"?\\n\" for help");
	
	//other initialization.
	state = WORKING;
	init_steppers();
	disable_steppers();
	
	// DEBUG
	FloatPoint p1 = { 3, 3 };
	FloatPoint p2 = { 0, 2 };
	print("DEBUG DEBUG \n p1 = ");
	printPoint(p1);
	print(", p2 = ");
	printPoint(p2);
	print("\n p1 + p2 = ");
	printPoint(p1 + p2);
	print("\nRotate p2 90 degrees: ");
	printPoint( p2.getRotatedRad(PI/2) );
	print("\n p1 normalize ");
	printPoint(p1.normalized());
	
	
}

//------------------------------------------------------------
void loop() {
	
	stateMachine();
	
	// while doing nothing make the delay go to max (slowest speed)
	current_delay += acceleration;
	current_delay = min( current_delay, max_delay);
	
}



//------------------------------------------------------------
void stateMachine() {
	switch (state) {
		case WORKING:
			checkSerial();
			break;
		case FILEWRITE:
			writeFromSerial();
			break;
	}
}


//------------------------------------------------------------
void goHome() {
	moveTo(current_pos.x, current_pos.y, 100); // lift brush on current position
	moveTo(0, 0, 100);
	disable_steppers();
}


//------------------------------------------------------------
void setHome() {
	// set the current position as 0,0,0
	set_position(0, 0, 0);
	offSet.x = 0;
	offSet.y = 0;
	println("Set new home position (x,y,z = 0,0,0 now)");
}

//------------------------------------------------------------
void moveTo(long x, long y) {
	moveTo(x, y, current_pos.z);
}

//------------------------------------------------------------
void moveTo(long x, long y, long z) {

	if ( bPreview ) {
		// calculate the distance
		travelDistance += (unsigned long) sqrt( (x - current_pos.x) * (x - current_pos.x) +
											   (y - current_pos.y) * (y - current_pos.y));
	}else{
		if( x < 0 ){ // should only happen when setting the home position / doing relative moves
			print("Warning!! New target X is ");
			println(x);
			return;
		}
		if( y < 0 ){
			print("Warning!! New target Y is ");
			println(y);
			return;
		}
		if( z < 0 && !(tool == TOOL_LIGHT)){ // when light z can go below 0
			print("Warning!! New target Z is ");
			println(z);
			return;
		}
	}
	set_target(x, y, z);
	dda_move(max_delay);
}

//-----------------------------------------------------------
void setLight(bool isOn){
	if(isOn && !bPreview){
		digitalWrite(LED_PIN, HIGH);	// switch off the light
	}else{
		digitalWrite(LED_PIN, LOW); // switch on the light
	}
}


//------------------------------------------------------------
// HELPERS
//------------------------------------------------------------
//------------------------------------------------------------
void printState() {
	
	printPos(current_pos);
	println();
	
	print(" Longhand ");
	println(VERSION);
	if (debug > 1) {
		println(" #####################");
		println(" ### in FULL DEBUG mode ### ");
		println(" #####################");
	}
	println(" -- STATE -- ");
	print(" max delay (slow): ");
	println(max_delay);
	
	print(" min delay (fast): ");
	println(min_delay);
	
	print(" acceleration: ");
	println(acceleration);
	
	print(" microSteps: ");
	println(microSteps);
	
	print(" scale: ");
	println( scale );
	
	print(" rotation: ");
	println( rotation );
	
	// position
	print(" Position:\n \t");
	print(current_pos.x);
	print(", ");
	print(current_pos.y);
	print(", ");
	print(current_pos.z);
	println(" steps");
	
	print(" \t");
	print((float)current_pos.x/mmToStep);
	print(", ");
	print((float)current_pos.y/mmToStep);
	print(", ");
	print((float)current_pos.z/mmToStep);
	print(" mm  (1mm = ");
	print(mmToStep);
	println(" steps)");
	print("\n knife radius: ");
	println(knifeRadius);
	
	print (" free ram: ");
	println ( freeRam());
	
	/*
	 // Just for documentation
	 println("");
	 println(" -- COMMANDS: --");
	 println(" a command ends with a newline character ('\\n')");
	 // info
	 
	 println("");
	 println(" - Moves");
	 println(" mx,y : moveto x, y");
	 println(" Mx,y,z : relative moveto x, y, z");
	 println(" lx,y : lineto x, y");
	 println(" ax1,y1,radius,beginAngle,angleDif : draw an arc");
	 println(" bx1,y1,ax1,ay1,ax2,ay2,x2,y2 : make bezier path ");
	 println(" i100 : set bezier resolution to 100. 1 = low res, 100 = high, 1000 = super high ");
	 println(" ex,y,r1,r2 : Ellipse at x,y with radius r1 (width/2) and r2(height/2)");
	 println(" o : Set this position as home/origin ");
	 println(" h : go home ");
	 println(" c : find home ");
	 println(" z1 : enable the Z axis stepper ");
	 
	 
	 println("");
	 println(" - Settings");
	 println(" d1 : set debug to 1 (level 1)");
	 println(" t1 : set bPreview to 1 (the machine will not move the steppers");
	 println(" r10 : set circle resolution to 10degrees / part");
	 println(" x50 : set scale to 50%");
	 println(" v2 : rotation = 2*90°");
	 println(" s1000,2000,5 : set speeds :min delay = 1000, max delay = 2000, acceleration = 5");
	 
	 println("");
	 println(" - Read Write Files");
	 println(" f : print the list of  .lhd files on the SD card");
	 println(" kFilename.lhd : delete Filename.lhd from the SD card");
	 println(" pFilename.lhd : start drawing from the file myfile.lhd on the SD card");
	 println(" wFilename.lhd : start writing from serial to Filename.lhd on SD, \\r (car. return) to end writing");
	 println("");
	 
	 */
}

//------------------------------------------------------------
void printPos(LongPoint p) {
	// print my current position to the serial port
	print("_p");
	print((float)p.x/mmToStep);
	print(" ");
	print((float)p.y/mmToStep);
	print(" ");
	println((float)p.z/mmToStep);
}


//------------------------------------------------------------
void printPoint(FloatPoint p) {
	print(p.x);
	print(", ");
	print(p.y);
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
extern char __bss_end;
#endif  // __arm__

//------------------------------------------------------------------------------
int freeRam() {
	// from https://github.com/greiman/SdFat
	char top;
#ifdef __arm__
	return &top - reinterpret_cast<char*>(sbrk(0));
#else  // __arm__
	return __brkval ? &top - __brkval : &top - &__bss_end;
#endif  // __arm__
}


