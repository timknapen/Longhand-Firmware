#include "Arduino.h"

#define MAX_PARSE_VALUES 9
// 9 is the max number of values I need to store for one single command. for example:
// b x1,y1, ax1,ay1, ax2,ay2, x2,y2, targetspeed  = 9 values

float fVals[MAX_PARSE_VALUES];	// float values
float lVals[MAX_PARSE_VALUES];	// long values

/****
 *
 * PARSED LETTERS
 *
 * a	Arc
 * b	Bezier
 * c	Callibrate (find home)
 * d	Debug       : set debug level
 * e	Ellipse
 * f	Files       : display a list of files
 * g
 * h	Home        : go to home position (0,0)
 * i	bezierResolution
 * j
 * k    Kill        : delete a file
 * l	Lineto      : put the pen down and go to position (x,y)
 * m	Moveto      : move the pen to position (x,y,z)
 * n	tool selection: 1 = Pen, 2 = Brush
 * o	Origin      : set as origin
 * p	Print       : draw a file
 * q
 * r	Radius		: knife radius
 * s	Speeds (delays)
 * t	Testrun
 * u	Microsteps
 * v    v : rotation (0,1,2,3 times 90 degrees)
 * w	Write to file
 * x	X times : scale the drawing from file
 * y
 * z    Enable/disable z axis
 *
 ****/


//------------------------------------------------------------
void parseMessage(char* input, int length) {
	if (length  < 1) { // BAD MESSAGE!
		print("BAD message: too short '");
		print(input);
		println("'");
		return;
	}
	float value = 0; // calculate number following the command
	if (length > 1) {
		value = atof(&input[1]); //<< length needs to be at least 2 to make this fail safe!
	}
	switch (input[0]) {
			// MOVES
		case 'A':
		case 'a':	// draw an arc
			parseArc(&input[1], length - 1);
			break;
			
		case 'B':
		case 'b':	// draw a bezier path
			parseBezier(&input[1], length - 1);
			break;
			
		case 'C':
		case 'c':	// find home
			findHome();
			break;
			
		case 'E':
		case 'e':	// draw an ellipse
			parseEllipse(&input[1], length - 1);
			break;
			
		case 'h':
		case 'H':	// go back home!
			goHome();
			break;
			
			
		case 'l':
		case 'L':	// Line to
			parseLineTo(&input[1], length - 1);
			break;
			
		case 'm':	// Move to
			parseMoveTo(&input[1], length - 1);
			break;
		case 'M':	// Relative move to
			parseRelativeMoveTo(&input[1], length - 1);
			break;
			
		case 'o':
		case 'O':	// set origin
			setHome();
			break;
			
			// SETTINGS
			
		case 'd':
		case 'D':	// toggle debug statements off / on / A LOT
			debug = (bool)value;
			switch (debug) {
				case 0:
					println("Debug is OFF");
					break;
				case 1:
					println("Debug is ON");
					break;
				case 2:
					println("Debug is ON HIGH ALERT");
					break;
			}
			break;
		case 'n':
		case 'N':
			tool = (int)value;
			switch (tool) {
				default:
					println("tool set to UNKNOWN");
					break;
				case 1:
					println("tool set to PEN");
					break;
				case 2:
					println("tool set to BRUSH");
					break;
				case 3:
					println("tool set to KNIFE");
					break;
			}
			break;
		case 'i':
		case 'I':	// set bezier resolution
			bezierResolution = (int)value;
			if (bezierResolution == 0.0f) bezierResolution = 1.0f;
			if (debug) {
				print("Bezier Resolution: ");
				println((int)bezierResolution);
			}
			break;
			
		case 'r':
		case 'R':	//set resolution for circles
			knifeRadius = value;
			if (knifeRadius < 0) knifeRadius = 1;
			if (debug) {
				print("knifeRadius = ");
				println(knifeRadius);
			}
			break;
			
		case 's':
		case 'S':	// set speeds
			parseDelays(&input[1], length - 1);
			break;
			
		case 't':	// switch bPreview on / off
		case 'T':
			bPreview = value;
			if (debug) {
				if (bPreview) {
					println("Machine is in preview mode");
				}
				else {
					println("Machine is in active mode");
				}
			}
			break;
			
		case 'u':
		case 'U':
			setMicroSteps(value);
			print("Microsteps set to 1/");
			println(value);
			break;
			
		case 'x':
		case 'X':	// set scale
			scale = value;
			print("Scale is ");
			println(scale);
			break;
			
		case 'v':
		case 'V':	// set rotation
			rotation = value;
			print("Rotation is ");
			switch (rotation) {
				case 0:
					println("Up");
					break;
				case 1:
					println("Right");
					break;
				case 2:
					println("Down");
					break;
				case 3:
					println("Left");
					break;
				default:
					break;
			}
			break;
			
			// READ WRITE FILES
			
		case 'f':
		case 'F':	// print a list of files to serial
			getFileList();
			break;
			
		case 'k':
		case 'K':
			// kill : delete a file!
			// the rest of the command should be the filename
			deleteFile(&input[1], length - 1);
			getFileList();
			break;
			
			
		case 'p':
		case 'P':
			// start drawing the selected file
			// the rest of the command should be the filename
			drawFromSD(&input[1], length - 1 );
			break;
			
		case 'w':
		case 'W':	// write the following to the file "PRINTJOB.LHD"
			startWritingToFile(&input[1], length - 1);
			break;
			
		case 'z':
		case 'Z':	// enable or disable the Z stepper
			if (value) {
				enable_z();
			}
			else {
				disable_z();
			}
			break;
			
			
			// INFO
		case '?':	// send me some info
			printState();
			break;
		default:	// bad command
			println("? bad command");
			break;
	}
	
}


//------------------------------------------------------------
int parseLongs(char * input, int length) {
	// parses the char * input with length
	// looks for comma separated longs
	// puts them in lVals and returns how many longs were found.
	int v = 0; // counter for values
	char c;
	if (length < 1) return 0;
	lVals[v] = atol(&input[0]);	// get the first value
	v++;
	for ( int i = 0; i < length - 1; i++) { // length-1 because we always parse from the position after the comma: atof(input[i+1])
		c = input[i];
		if ( c == ',' && v < MAX_PARSE_VALUES) {
			lVals[v] = atof(&input[i + 1]);
			v ++;
		}
	}
	return v;
}


//------------------------------------------------------------
int parseFloats(char * input, int length) {
	// parses the char * input with length
	// looks for comma separated integers
	// puts them in fVals and returns how many floats were found.
	int v = 0; // counter for values
	char c;
	if (length < 1) return 0;
	fVals[v] = atof(&input[0]); // this is fVals[0]
	v++;
	for ( int i = 0; i < length - 1; i++) { // length-1 because we always parse from the position after the comma: atof(input[i+1])
		c = input[i];
		if ( c == ',' && v < MAX_PARSE_VALUES) {
			fVals[v] = atof(&input[i + 1]);
			v ++;
		}
	}
	return v;
}


//------------------------------------------------------------
void parseDelays(char * mssg, int length) {
	int valnum =  parseLongs(mssg, length);
	// I want a min delay and a max delay and an acceleration
	if ( valnum < 3) return;
	min_delay = lVals[0];		// the fastest speed possible
	max_delay = lVals[1];		// the slowest speed possible.
	acceleration = lVals[2];	// gets added each step to the delay to calculate the acceleration speed
	if (debug) {
		print("min delay: ");
		println(min_delay);
		print("max_delay: ");
		println(max_delay);
		print("acceleration: ");
		println(acceleration);
	}
}


//------------------------------------------------------------
void parseBezier( char * mssg, int length) {
	int valnum =  parseFloats(mssg, length);
	if ( valnum < 8) return;
	setLight(true);
	bezier((long)(mmToStep * scale * fVals[0]),
		   (long)(mmToStep * scale * fVals[1]),
		   (long)(mmToStep * scale * fVals[2]),
		   (long)(mmToStep * scale * fVals[3]),
		   (long)( mmToStep * scale * fVals[4]),
		   (long)(mmToStep * scale * fVals[5]),
		   (long)( mmToStep * scale * fVals[6]),
		   (long)(mmToStep * scale * fVals[7]));
	setLight(false);
}


//------------------------------------------------------------
void parseArc(char* mssg, int length) {
	int valnum =  parseFloats(mssg, length);
	if (valnum < 5) return;
	// arc(x,y, radius, beginAngle, endAngle )
	setLight(true);
	arc((long)(mmToStep * scale * fVals[0]),
		(long)(mmToStep * scale * fVals[1]),
		(long)(mmToStep * scale * fVals[2]),
		(long)fVals[3],
		(long)fVals[4] );
	setLight(false);

}


//------------------------------------------------------------
void parseEllipse(char* mssg, int length) {
	int valnum =  parseFloats(mssg, length);
	if (valnum < 4) return;
	setLight(true);
	ellipse((long)(mmToStep * scale * fVals[0]),
			(long)(mmToStep * scale * fVals[1]),
			(long)(mmToStep * scale * fVals[2]),
			(long)(mmToStep * scale * fVals[3]));
	setLight(false);

}


//------------------------------------------------------------
void parseMoveTo(char* mssg, int length) {
	setLight(false);

	int valnum =  parseFloats(mssg, length);
	if (valnum < 2) return;
	float x = fVals[0];
	float y = fVals[1];
	float z = 100;
	if(valnum >= 3){
		z = fVals[2];
	}
	
	if (isDrawingFromFile) { // only scale and rotate when drawing from file
		switch (rotation) {
			case 1: // 90
				x = - fVals[1];
				y = fVals[0];
				break;
			case 2: // 180
				x = - fVals[0];
				y = - fVals[1];
				break;
			case 3: // 270
				x = fVals[1];
				y = - fVals[0];
				break;
		}
		
		x *= scale;
		y *= scale;
		
	}
	
	if(tool == TOOL_KNIFE && isDrawingFromFile){ // move to target + knifedir
		knifePos.set(x,y);
		x += knifeDir.x;
		y += knifeDir.y;
	}
	
	x *= mmToStep;
	y *= mmToStep;
	
	if( tool == TOOL_LIGHT ){
		moveTo(offSet.x + (long)x, offSet.y + (long)y, (long)z);
		return;
	}
	if ( current_pos.x == (long)x && current_pos.y == (long)y) {
		// it's a useless moveTo!
		return;
	}
	moveTo((long)(current_pos.x), (long)(current_pos.y), z); // brush up at current position
	moveTo(offSet.x + (long)x, offSet.y + (long)y, z);
}


//------------------------------------------------------------
void parseRelativeMoveTo(char* mssg, int length) {
	setLight(false);

	int valnum =  parseFloats(mssg, length);
	if (valnum < 2) return;
	float x = fVals[0];
	float y = fVals[1];
	float z = 0;
	if (valnum >= 3) { // z was not set, so use 0
		z = fVals[2];
	}
	
	if (isDrawingFromFile) { // only scale and rotate when drawing from file
		switch (rotation) {
			case 1: // 90
				x = - fVals[1];
				y = fVals[0];
				break;
			case 2: // 180
				x = - fVals[0];
				y = - fVals[1];
				break;
			case 3: // 270
				x = fVals[1];
				y = - fVals[0];
				break;
		}
		
		x *= scale;
		y *= scale;
		
	}
	
	x *= mmToStep;
	y *= mmToStep;
	moveTo(current_pos.x + (long)x, current_pos.y + (long)y, current_pos.z + (long)z);
}

//------------------------------------------------------------
void parseLineTo(char* mssg, int length) {
	int valnum =  parseFloats(mssg, length);
	if (valnum < 2) return;
	float x = fVals[0];
	float y = fVals[1];
	float z = 0;
	if (valnum < 3) { // z was not set, so use 0
		z = 0;
	}else{
		z = fVals[2];
	}
	
	if (isDrawingFromFile) { // only scale and rotate when drawing from file
		switch (rotation) {
			case 1: // 90
				x = - fVals[1];
				y = fVals[0];
				break;
			case 2: // 180
				x = - fVals[0];
				y = - fVals[1];
				break;
			case 3: // 270
				x = fVals[1];
				y = - fVals[0];
				break;
		}
		
		x *= scale;
		y *= scale;
		
	}
	
	// put the pen down if necessary, but only when no Z was defined...
	if (current_pos.z != (long)z && valnum < 3 && tool != TOOL_LIGHT ) {
		moveTo(current_pos.x, current_pos.y, (long)z);
	}
	
	// when using the knife rotate to the new direction first!
	if(tool == TOOL_KNIFE && isDrawingFromFile){ // move to target + knifedir
		FloatPoint next = { x, y }; // our target
		FloatPoint cur = knifePos;
		float a1 = atan2(knifeDir.y, knifeDir.x);
		knifeDir  = ((next - cur).normalized()) * knifeRadius;
		float a2 = atan2(knifeDir.y, knifeDir.x);
		float adif = angleDif(a1, a2);
		
		FloatPoint r = {knifeRadius, 0};
		float stepAngle = TWO_PI/18.0f; // 20° steps
		if(adif < 0){
			stepAngle *= -1;
		}
		int circleParts = abs(adif/stepAngle);
		FloatPoint p;
		for(int j = 1; j < circleParts; j++){
			p = cur + r.getRotatedRad(a1 + (float)j*stepAngle);
			moveTo(offSet.x + (long)(p.x * mmToStep), offSet.y + (long)(p.y * mmToStep), (long)z);
		}
		p = cur + knifeDir;
		moveTo(offSet.x + (long)(p.x * mmToStep), offSet.y + (long)(p.y * mmToStep), (long)z);
		
		// add overshoot
		x += knifeDir.x;
		y += knifeDir.y;
		knifePos = next;
	}
	
	
	x *= mmToStep;
	y *= mmToStep;
	
	setLight(true);
	moveTo(offSet.x + (long)x, offSet.y + (long)y, (long)z);
	setLight(false);

}


//--------------------------------------------------------------
float angleDif(float angle1, float angle2){
	
	// solving this once and for all
	// angle 1 turns to angle 2 (clockwise
	// if angle 1 = 170 && angle 2 = -170 => distance = +20
	// BUT angle 2 - angle 1 = 340!!!
	// angle 1 & 2 in radians!
	// OUTPUT IN RADIANS : from -PI to +PI
	float normal =	angle2 - angle1;
	float anormal =	fabsf(normal);
	float plus =	angle2 - angle1 + TWO_PI;
	float aplus =	fabsf(plus);
	float min =		angle2 - angle1 - TWO_PI;
	float amin =	fabsf(min);
	if(amin < aplus){
		if(amin < anormal){
			return min;
		}else  {
			return normal;
		}
	}else {
		if(aplus < anormal){
			return plus;
		}else {
			return normal;
		}
	}
}




