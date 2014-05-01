#include <SPI.h>
#include <SD.h>
#include "parameters.h"
#include "pins.h"
#include "LongPoint.h"
#include "Arduino.h"

/*------------------------------------------------------------
 
 LONGHAND DRAWING MACHINE firmware V2.2
 
 last update 01/05/2014
 by Tim Knapen
 http://www.longhand.cc/
 
 This is the firmware for
 + Arduino Due
 + Longhand PCB V1
 + 4 Pololu A4988 stepper motor driver carriers
 
 Compiles with Arduino IDE 1.5.2
 
 ------------------------------------------------------------*/

// SERIAL
//#define BAUD 115200			// serialUSB doesn't need a baud rate !
#define bufferLength 64                     // serial buffer length
char serialBuffer[bufferLength];            // serial buffer
int iSerialBuf = 0;							// position in the serialBuffer
#define endline '\n'						// a command always ends with a newline '\n'

// BEZIER
float bezierResolution = 10.0f;				// in how many straight lines will I approximate a bezier curve?
float circleRes = 0.5f;						// circleResolution

// STATES
#define WORKING 1							// listening to serial, taking commands
#define FILEWRITE 2							// not taking commands, moving everything I receive from serial onto a file.
int state = WORKING;

// DEBUG
bool debug = false;                         // output debug statements
bool testrun = true;						// to check if I'm doing a testrun

// SPEEDS
int current_delay = 1000;					// in microsecs
int min_delay = 1000;						// the fastest speed possible
int max_delay = 3000;						// the slowest speed possible.
int acceleration = 10;						// gets added each step to the delay to calculate the acceleration speed
int microSteps = 8;							// the type of microsteps we are taking, default is 1/8th step

// DISTANCE (keep track of how long a print will take)
long travelDistance = 0;

// POSITIONS (for steppers)
LongPoint current_pos;						// current position in steps
LongPoint target_pos;						// targent position in steps
LongPoint delta_steps;						// the distances on each axis
LongPoint offSet;

// scale existing drawings to new stepper resolutions
int scale = 1;
int rotation = 0; // in 90° : 1 = 90, 2 = 180, 3 = -90

//------------------------------------------------------------
void setup(){
	pinMode(9, OUTPUT);		// I'm using the WifiShield for the microSD card reader it has
	digitalWrite(9, HIGH);	// switch on the LED on the wifiShield
	while(!SerialUSB); // wait for the serialUSB to come up
    
    /* TEST **/
    
    /* **/
	//other initialization.
	init_steppers();
    
	SerialUSB.println("Longhand Drawing Machine V2.2 awaiting commands");
	SerialUSB.println("Send me \"?\\n\" for help");
	
#ifdef FULLDEBUG
	SerialUSB.println("In FULLDEBUG mode");
#endif
	
	getFileList();
	state = WORKING;
	disable_steppers();
	
	digitalWrite(9, LOW);
}

//------------------------------------------------------------
void loop(){
    
	stateMachine();
	
	// while doing nothing make the delay go to max (slowest speed)
	current_delay += acceleration;
	current_delay = min( current_delay, max_delay);
	
}



//------------------------------------------------------------
void stateMachine(){
	switch(state){
		case WORKING:
			checkSerial();
			break;
		case FILEWRITE:
			writeFromSerial();
			break;
	}
}


//------------------------------------------------------------
void goHome(){
	moveTo(current_pos.x, current_pos.y, 100); // lift brush on current position
	moveTo(0, 0, 100);
	disable_steppers();
}


/*
 //------------------------------------------------------------
 void setHome(){
 // set the current position as 0,0,0
 // set_position(0, 0, 0);
 offSet.x = 0;
 offSet.y = 0;
 SerialUSB.println("Set new Offset position (x,y,z = 0,0,0 now)");
 }
 */

//------------------------------------------------------------
void moveTo(long x, long y){
    moveTo(x, y, current_pos.z);
}

//------------------------------------------------------------
void moveTo(long x, long y, long z){
	//*
    if( (x < 0 || y < 0 || z < 0  || z > 200) && !testrun){
		// should only happen when setting the home position / doing relative moves
		SerialUSB.print("Warning!! new target is ");
		SerialUSB.print(x, DEC);
		SerialUSB.print(", ");
		SerialUSB.print(y, DEC);
        SerialUSB.print(", ");
        SerialUSB.println(z , DEC);
		return;
	}
    //*/
	set_target(x, y, z);
	dda_move(max_delay);
}


//------------------------------------------------------------
// HELPERS
//------------------------------------------------------------
//------------------------------------------------------------
void printState(){
	
	SerialUSB.println(" Longhand V2.2 ");
	SerialUSB.println(" -- STATE -- ");
	SerialUSB.print(" max delay (slow): ");
	SerialUSB.println(max_delay);
	SerialUSB.print(" min delay (fast): ");
	SerialUSB.println(min_delay);
	SerialUSB.print(" acceleration: ");
	SerialUSB.println(acceleration);
	SerialUSB.print(" microSteps: ");
	SerialUSB.println(microSteps);
	SerialUSB.print(" scale: ");
	SerialUSB.println( scale );
	SerialUSB.print(" rotation: ");
	SerialUSB.println( rotation );
	// position
	SerialUSB.print(" Position:    ");
	SerialUSB.print(current_pos.x);
	SerialUSB.print(" x, ");
	SerialUSB.print(current_pos.y);
	SerialUSB.print(" y, ");
	SerialUSB.print(current_pos.z);
	SerialUSB.println(" z");
	
	SerialUSB.print(" Circle resolution (x10): ");
	SerialUSB.println(10*circleRes);
	
	SerialUSB.print(" Bezier resolution: ");
	SerialUSB.print( bezierResolution);
	
	/*
     // Just for documentation
     SerialUSB.println("");
     SerialUSB.println(" -- COMMANDS: --");
     SerialUSB.println(" a command ends with a newline character ('\\n')");
     // info
     
     SerialUSB.println("");
     SerialUSB.println(" - Moves");
     SerialUSB.println(" mx,y : moveto x, y");
     SerialUSB.println(" Mx,y,z : relative moveto x, y, z");
     SerialUSB.println(" lx,y : lineto x, y");
     SerialUSB.println(" ax1,y1,radius,beginAngle,angleDif : draw an arc");
     SerialUSB.println(" bx1,y1,ax1,ay1,ax2,ay2,x2,y2 : make bezier path ");
     SerialUSB.println(" i100 : set bezier resolution to 100. 1 = low res, 100 = high, 1000 = super high ");
     SerialUSB.println(" ex,y,r1,r2 : Ellipse at x,y with radius r1 (width/2) and r2(height/2)");
     SerialUSB.println(" o : Set this position as home/origin ");
     SerialUSB.println(" h : go home ");
     SerialUSB.println(" c : find home ");
     SerialUSB.println(" z1 : enable the Z axis stepper ");
     
     
     SerialUSB.println("");
     SerialUSB.println(" - Settings");
     SerialUSB.println(" d1 : set debug to 1 (on)");
     SerialUSB.println(" t1 : set testrun to 1 (the machine will not move the steppers");
     SerialUSB.println(" r10 : set circle resolution to 10degrees / part");
     SerialUSB.println(" x50 : set scale to 50%");
     SerialUSB.println(" v2 : rotation = 2*90°");
     SerialUSB.println(" s1000,2000,5 : set speeds :min delay = 1000, max delay = 2000, acceleration = 5");
     
     SerialUSB.println("");
     SerialUSB.println(" - Read Write Files");
     SerialUSB.println(" f : print the list of  .lhd files on the SD card");
     SerialUSB.println(" k : kill all files on SD card");
     SerialUSB.println(" pFilename.lhd : start drawing from the file myfile.lhd on the SD card");
     SerialUSB.println(" wFilename.lhd : start writing from serial to Filename.lhd on SD, \\r (car. return) to end writing");
     SerialUSB.println("");
     
     */
}

//------------------------------------------------------------
void printPos(long x, long y, long z){
	// print my current position to the serial port
	SerialUSB.print("_p");
	SerialUSB.print(x);
	SerialUSB.print(" ");
	SerialUSB.print(y);
	SerialUSB.print(" ");
	SerialUSB.println(z);
	delayMicroseconds(5);
}