// Longhand Firmware

#include "Arduino.h"

#define MAX_PARSE_VALUES 9
// 9 is the max number of values I need to store for one single command. for example:
// b x1,y1, ax1,ay1, ax2,ay2, x2,y2, targetspeed  = 9 values

float fVals[MAX_PARSE_VALUES];	// float values
float lVals[MAX_PARSE_VALUES];	// long values

/****
 
 PARSED LETTERS
 
 a	arc
 b	bezier
 c	callibrate (find home
 d	debug
 e	ellipse
 f	files
 g
 h	home
 i	bezierResolution
 j
 k
 l	lineto
 m	moveto
 n
 o	origin
 p	draw
 q
 r	circleRes
 s	speeds (delays)
 t	testrun
 u	microsteps
 v
 w	writeToFile
 x	scale
 y
 z
 
 ****/


//------------------------------------------------------------
void parseMessage(char* input, int length){
	if(length >0){ // message received!
		int value = 0; // calculate number following the command
		value = atol(&input[1]);
		switch(input[0]){
				
				// MOVES
				
			case 'A':
			case 'a':	// draw an arc
				parseArc(&input[1], length-1);
				break;
				
			case 'B':
			case 'b':	// draw a bezier path
				parseBezier(&input[1], length-1);
				break;
			
			case 'C':
			case 'c':	// find home
				findHome();
				break;
				
			case 'E':
			case 'e':	// draw an ellipse
				parseEllipse(&input[1], length-1);
				break;
				
			case 'h':
			case 'H':	// go back home!
				goHome();
				break;
				
			case 'l':
			case 'L':	// Line to
				parseLineTo(&input[1], length-1);
				break;
				
			case 'm':	// Move to
				parseMoveTo(&input[1], length-1);
			case 'M':	// Relative move to
				parseRelativeMoveTo(&input[1], length-1);
				break;
				
			case 'o':
			case 'O':	// set origin
				setHome();
				break;
				
				
				// SETTINGS
				
			case 'd':
			case 'D':	// toggle debug statements on / off
				debug = value;
				if(debug){
					SerialUSB.println("Debug is ON");
				}
				break;
				
			case 'i':
			case 'I':	// set bezier resolution
				bezierResolution = value;
				if(bezierResolution == 0.0f) bezierResolution = 1.0f;
				if(debug){
					SerialUSB.print("Bezier Resolution: ");
					SerialUSB.println((int)bezierResolution);
				}
				break;
				
			case 'r':
			case 'R':	//set resolution for circles
				circleRes = value;
				if(circleRes < 0.1) circleRes = 1;
				if(debug){
					SerialUSB.print("Circle resolution (x10)= ");
					SerialUSB.println(10*circleRes);
				}
				break;
				
			case 's':
			case 'S':	// set speeds
				parseDelays(&input[1], length-1);
				break;
				
			case 't':	// switch testrun on / off
			case 'T':
				testrun = value;
				if(debug){
					if(testrun){
						SerialUSB.println("machine is doing testruns");
					}else{
						SerialUSB.println("machine is drawing");
					}
				}
				break;
				
			case 'u':
			case 'U':
				setMicroSteps(value);
				SerialUSB.print("microsteps set to 1/");
				SerialUSB.println(value);
				break;
			
			case 'x':
			case 'X':	// set scale
				scale = value;
				SerialUSB.print("scale is ");
				SerialUSB.println(scale);
				break;
				
				// READ WRITE FILES
				
			case 'f':
			case 'F':	// print a list of files to serial
				getFileList();
				break;
				
			case 'p':
			case 'P':
				// start drawing the selected file
				// the rest of the command should be the filename
				drawFromSD(&input[1], length -1 );
				break;

			case 'w':
			case 'W':	// write the following to the file "PRINTJOB.LHD"
				startWritingToFile(&input[1], length -1);
				state = FILEWRITE;
				break;
				
			
				
				// INFO
				
			case '?':	// send me some info
				printState();
				break;
			default:	// bad command
				SerialUSB.println("? bad command");
				break;
		}
	}
}


//------------------------------------------------------------
int parseLongs(char * input, int length){
	// parses the char * input with length
	// looks for comma separated longs
	// puts them in lVals and returns how many integers were found.
	int v = 0; // counter for values
	char c;
	if(length < 1) return 0;
	lVals[v] = atol(&input[0]);	// get the first value
	v++;
	for( int i = 0; i < length-1; i++){ // length-1 because we always parse from the position after the comma: atol(input[i+1])
		c = input[i];
		if( c == ',' && v < MAX_PARSE_VALUES){
			lVals[v] = atol(&input[i+1]);
			v ++;
		}
	}
	return v;
}


//------------------------------------------------------------
int parseFloats(char * input, int length){
	// parses the char * input with length
	// looks for comma separated integers
	// puts them in fVals and returns how many floats were found.
	int v = 0; // counter for values
	char c;
	if(length < 1) return 0;
	fVals[v] = atof(&input[0]); // this is fVals[0]
	v++;
	for( int i = 0; i < length-1; i++){ // length-1 because we always parse from the position after the comma: atof(input[i+1])
		c = input[i];
		if( c == ',' && v < MAX_PARSE_VALUES){
			fVals[v] = atof(&input[i+1]);
			v ++;
		}
	}
	return v;
}


//------------------------------------------------------------
void parseDelays(char * mssg, int length){
	int valnum =  parseLongs(mssg, length);
	// I want a min delay and a max delay and an acceleration
	if( valnum < 3) return;
	min_delay = lVals[0];		// the fastest speed possible
	max_delay = lVals[1];		// the slowest speed possible.
	acceleration = lVals[2];	// gets added each step to the delay to calculate the acceleration speed
	if(debug){
		SerialUSB.print("min delay: ");
		SerialUSB.println(min_delay);
		SerialUSB.print("max_delay: ");
		SerialUSB.println(max_delay);
		SerialUSB.print("acceleration: ");
		SerialUSB.println(acceleration);
	}
}


//------------------------------------------------------------
void parseBezier( char * mssg, int length){
	int valnum =  parseLongs(mssg, length);
	if( valnum < 8) return;
	bezier( scale * lVals[0], scale * lVals[1],
			scale * lVals[2], scale * lVals[3],
			scale * lVals[4], scale * lVals[5],
			scale * lVals[6], scale * lVals[7] );
}


//------------------------------------------------------------
void parseArc(char* mssg, int length){
	int valnum =  parseFloats(mssg, length);
	if(valnum < 5) return;
	// arc(x,y, radius, beginAngle, endAngle )
	arc(	scale * fVals[0], scale * fVals[1], scale * fVals[2], fVals[3], fVals[4] );
}


//------------------------------------------------------------
void parseEllipse(char* mssg, int length){
	int valnum =  parseLongs(mssg, length);
	if(valnum < 4) return;
	ellipse(scale * lVals[0], scale * lVals[1], scale * lVals[2], scale * lVals[3]);
}


//------------------------------------------------------------
void parseMoveTo(char* mssg, int length){
	int valnum =  parseLongs(mssg, length);
	if(valnum < 2) return;
	moveTo(current_pos.x, current_pos.y, 50); // brush up at current position
	moveTo(scale * lVals[0], scale * lVals[1], 50); // keep the brush up
}


//------------------------------------------------------------
void parseRelativeMoveTo(char* mssg, int length){
	int valnum =  parseLongs(mssg, length);
	if(valnum < 3 ) return;
	moveTo(current_pos.x + scale * lVals[0], current_pos.y + scale * lVals[1], current_pos.z + scale * lVals[2]);
}

//------------------------------------------------------------
void parseLineTo(char* mssg, int length){
	int valnum =  parseLongs(mssg, length);
	if(valnum >= 3){ // with z
		moveTo(current_pos.x, current_pos.y, scale * lVals[2]);
		moveTo(scale * lVals[0], scale * lVals[1], scale * lVals[2]);
	}else if(valnum == 2){
		moveTo(current_pos.x, current_pos.y, 0);
		moveTo(scale * lVals[0], scale * lVals[1], 0);
	}
}





