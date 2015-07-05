#include "pins.h"
#include "Arduino.h"
#include "LongPoint.h"


// Our direction vars
byte x_direction = 1;
byte y_direction = 1;
byte z_direction = 1;



// TIMING
int milli_delay;			// helper for switching between delayMicroseconds(); and delay();

//---------------------------------------------------------------------------------------
void init_steppers()
{
  	// init our points.
  	current_pos.x = 0;
  	current_pos.y = 0;
  	current_pos.z = 0;
  	
  	target_pos.x = 0;
  	target_pos.y = 0;
  	target_pos.z = 0;
	
	offSet.x = 0;
	offSet.y = 0;
  	offSet.z = 0;
	
  	// set the pins for the stepper drivers
  	pinMode(X_STEP_PIN, OUTPUT);
  	pinMode(X_DIR_PIN, OUTPUT);
  	pinMode(X_ENABLE_PIN, OUTPUT);
  	
  	pinMode(Y_STEP_PIN, OUTPUT);
  	pinMode(Y_DIR_PIN, OUTPUT);
  	pinMode(Y_ENABLE_PIN, OUTPUT);
  	
  	pinMode(Z_STEP_PIN, OUTPUT);
  	pinMode(Z_DIR_PIN, OUTPUT);
  	pinMode(Z_ENABLE_PIN, OUTPUT);
	//keep Z high!
  	
  	// pinMode(MS1_PIN, OUTPUT);
  	// pinMode(MS2_PIN, OUTPUT);
  	// pinMode(MS3_PIN, OUTPUT);
  	
  	
  	// set the endstops
  	pinMode(X_MIN_PIN, INPUT);
  	pinMode(Y_MIN_PIN, INPUT);
  	pinMode(Z_MIN_PIN, INPUT);
  	digitalWrite(X_MIN_PIN, HIGH); // set internal 100 KOhm pull up resistor
  	digitalWrite(Y_MIN_PIN, HIGH);
  	digitalWrite(Z_MIN_PIN, HIGH);
  	
  	
  	// get the microSteps pins set
  	// default is 1/8th steps
  	setMicroSteps(8);
  	
  	// turn them off to start.
  	disable_steppers();
  	
  	// figure out directions
  	calculate_deltas();
}


//------------------------------------------------
void calculate_deltas()
{
  	// distances in steps
  	delta_steps.x = abs(target_pos.x - current_pos.x);
  	delta_steps.y = abs(target_pos.y - current_pos.y);
  	delta_steps.z = abs(target_pos.z - current_pos.z);
  	
  	// what is our direction
  	x_direction = (target_pos.x >= current_pos.x);
  	y_direction = (target_pos.y >= current_pos.y);
  	z_direction = (target_pos.z >= current_pos.z);
  	
  	// set our direction pins
#if INVERT_X_DIR == 1
  	digitalWrite(X_DIR_PIN, !x_direction);
#else
  	digitalWrite(X_DIR_PIN, x_direction);
#endif
  	
#if INVERT_Y_DIR == 1
  	digitalWrite(Y_DIR_PIN, !y_direction);
#else
  	digitalWrite(Y_DIR_PIN, y_direction);
#endif
  	
#if INVERT_Z_DIR == 1
  	digitalWrite(Z_DIR_PIN, !z_direction);
#else
  	digitalWrite(Z_DIR_PIN, z_direction);
#endif
}

//------------------------------------------------
void findHome(){
  	
	long target_delay = min_delay;
  	enable_steppers();
	
	moveTo(current_pos.x, current_pos.y, current_pos.z + 100); // lift brush on current position
	
  	bool x_can_step = 1;
  	bool y_can_step = 1;
  	
  	x_direction = 0; // go back
  	y_direction = 0; // go back
  	
  	// set our direction pins
#if INVERT_X_DIR == 1
  	digitalWrite(X_DIR_PIN, !x_direction);
#else
  	digitalWrite(X_DIR_PIN, x_direction);
#endif
  	
#if INVERT_Y_DIR == 1
  	digitalWrite(Y_DIR_PIN, !y_direction);
#else
  	digitalWrite(Y_DIR_PIN, y_direction);
#endif
  	
  	x_can_step = checkEndstop( X_MIN_PIN, x_direction );
  	y_can_step = checkEndstop( Y_MIN_PIN, y_direction );
  	
  	int debounceCounter = 0;
  	
  	while ((x_can_step || y_can_step ) && debounceCounter < 10){
  		
  		if(x_can_step){
  			current_pos.x--;
  		}
  		if(y_can_step){
  			current_pos.y--;
  		}
  		
  		if(x_can_step || y_can_step){
  			debounceCounter = 0;
  		}else{
  			debounceCounter ++;
  		}
  		
  		step_axes(x_can_step, y_can_step, false);
  		
  		// current_delay = max_delay;
  		// how long do we delay for?
  		long acceleration_delay = current_delay - acceleration;
		current_delay = max( min_delay, acceleration_delay);
		
		if (current_delay >= 16383){
			milli_delay = current_delay / 1000;
			delay(milli_delay);
		}
		else
		{
			delayMicroseconds(current_delay);
		}
		
  		//printPos(current_pos);
  		
  		x_can_step = checkEndstop( X_MIN_PIN, x_direction );
  		y_can_step = checkEndstop( Y_MIN_PIN, y_direction );
  		
  		if(SerialUSB.available() > 0) {
  			println("\n calibration stopped by user");
  			break; // break out of while loop
  		}
  	}
  	println(MACHINE_STOPPED);
  	set_position(0,0, 100);
  	disable_steppers();
	printPos(current_pos);
}


//------------------------------------------------
bool checkEndstop( byte endstop_pin, int direction ){
  	// returns true if axis is allow to continue
  	// endstop_pin = pin used to read the endstop
  	// int direction = direction the axis wants to go in 0 = negative direction, 1 = positive direction
  	
  	// On Arduino Due the end stop pins are pulled up by an internal 100HOhm resistor.
  	// When the switch is pressed it is pulled to GND
  	bool axis_can_run = (digitalRead(endstop_pin) == HIGH); // is axis at min position?
  	
  	// check if we are going away from the switch (allowed)
  	// if dir is positive I don't mind you bumping into the endstop
  	if( direction )		axis_can_run = true;
  	
  	
  	return axis_can_run;
}


//------------------------------------------------
// adopted and fine tuned dda move from the old Reprap Darwin firmware.
void dda_move(long micro_delay)
{
  	
  	long target_delay = micro_delay;
  	calculate_deltas();
  	
  	if(!bPreview){
  		// turn on steppers to start moving
  		enable_steppers();
  	}
  	if(!bPreview && current_pos.z == 0 && target_pos.z == 0 && (tool == TOOL_PEN || tool == TOOL_KNIFE)){
		// let Z fall down if it is at 0 - only in PEN or KNIFE mode!
		disable_z();
	}
  	// figure out our deltas
  	long max_delta = max(max(delta_steps.x, delta_steps.y), delta_steps.z);
  	long current_delta = max_delta; // for calculating acceleration
  	
  	// init stuff.
  	long x_counter = -max_delta/2;
  	long y_counter = -max_delta/2;
  	long z_counter = -max_delta/2;
  	
  	
  	// our step flags
  	bool x_can_step = 0;
  	bool y_can_step = 0;
  	bool z_can_step = 0;
  	
  	long lastx = current_pos.x;
  	long lasty = current_pos.y;
  	long lastz = current_pos.z;
  	
  	
  	// do our DDA line!
	
  	x_can_step = (current_pos.x != target_pos.x) && checkEndstop( X_MIN_PIN, x_direction ); // added endstop check
  	y_can_step = (current_pos.y != target_pos.y) && checkEndstop( Y_MIN_PIN, y_direction ); // added endstop check
  	z_can_step = (current_pos.z != target_pos.z);
	
  	if(max_delta == 0){ // no move, do nothing
  		if(debug > 1){
  			println("dda_move 0 distance!:");
			print(current_pos.x);
			print(",");
			print(current_pos.y);
			print(",");
			print(current_pos.z);
			print(" delta: ");
			print(delta_steps.x);
			print(",");
			print(delta_steps.y);
			print(",");
			println(delta_steps.z);
  		}
  		return;
  	}
  	
  	bool x_needs_to_step, y_needs_to_step, z_needs_to_step;
  	
  	while (x_can_step || y_can_step || z_can_step ){
  		x_needs_to_step = y_needs_to_step = z_needs_to_step = false;
  		if (x_can_step)
  		{
  			x_counter += delta_steps.x;
  			
  			if (x_counter > 0)
  			{
  				x_needs_to_step = true;
  				x_counter -= max_delta;
  				
  				if (x_direction)
  					current_pos.x++;
  				else
  					current_pos.x--;
  			}
  		}
  		
  		if (y_can_step)
  		{
  			y_counter += delta_steps.y;
  			if (y_counter > 0)
  			{
  				y_needs_to_step = true;
  				y_counter -= max_delta;
  				
  				if (y_direction)
  					current_pos.y++;
  				else
  					current_pos.y--;
  			}
  		}
  		
  		if (z_can_step)
  		{
  			z_counter += delta_steps.z;
  			if (z_counter > 0)
  			{
  				z_needs_to_step = true;
  				z_counter -= max_delta;
  				
  				if (z_direction)
  					current_pos.z++;
  				else
  					current_pos.z--;
  			}
  		}
  		
  		step_axes(x_needs_to_step,
  				  y_needs_to_step,
  				  z_needs_to_step);
  		
  		
  		
  		// calculate delay.
  		if(!bPreview){
  			long acceleration_delay = current_delay - acceleration;
  			long decceleration_delay = target_delay - acceleration * current_delta;
  			current_delay = max( min_delay, max(acceleration_delay, decceleration_delay));
  			
  			// how long do we delay for?
  			if (current_delay >= 16383){
  				milli_delay = current_delay / 1000;
  				delay(milli_delay);
  			}
  			else
  			{
  				delayMicroseconds(current_delay);
  			}
  		}
  		// now I should have done a step and should be able to print out the position
		if(debug > 1){
			printPos(current_pos);
		}
  		current_delta --;
  		
  		x_can_step = (current_pos.x != target_pos.x) && checkEndstop( X_MIN_PIN, x_direction ); // added endstop check
		y_can_step = (current_pos.y != target_pos.y) && checkEndstop( Y_MIN_PIN, y_direction ); // added endstop check
  		z_can_step = (current_pos.z != target_pos.z);
		
		//checkSerial();
  	}
  	
  	// this is probably not very useful
  	// before removing this part, test it with a debug statement
  	if( (current_pos.x != target_pos.x ||
		 current_pos.y != target_pos.y ||
		 current_pos.z != target_pos.z ) && !bPreview){
  		println("ERROR - the dda move didn't finish properly");
  	}
  	current_pos.x = target_pos.x;
  	current_pos.y = target_pos.y;
  	current_pos.z = target_pos.z;
  	
  	if(debug){
  		printPos(current_pos);
  	}
  	
}


//-------------------------------------------------------------------
inline void step_axes(bool x_needs_to_step, bool y_needs_to_step, bool z_needs_to_step){
  	if(!bPreview){
  		if( x_needs_to_step) digitalWrite(X_STEP_PIN, HIGH);
  		if( y_needs_to_step) digitalWrite(Y_STEP_PIN, HIGH);
  		if( z_needs_to_step) digitalWrite(Z_STEP_PIN, HIGH);
  		delayMicroseconds(5); // A4988 needs at least 1 microsecond to register a step
  		if( x_needs_to_step) digitalWrite(X_STEP_PIN, LOW);
  		if( y_needs_to_step) digitalWrite(Y_STEP_PIN, LOW);
  		if( z_needs_to_step) digitalWrite(Z_STEP_PIN, LOW);
  	}
}

//---------------------------------------------------------------------------------------
void set_target(long x, long y)
{
  	
  	set_target(x , y, current_pos.z);
}

//---------------------------------------------------------------------------------------
void set_target(long x, long y, long z)
{
  	target_pos.x = x;
  	target_pos.y = y;
  	target_pos.z = z;
}


//---------------------------------------------------------------------------------------
void set_position(long x, long y )
{
  	set_position(x, y, current_pos.z);
}

//-------------------------------------------------------------------
void set_position(long x, long y, long z )
{
  	current_pos.x = x;
  	current_pos.y = y;
  	current_pos.z = z;
  	set_target(x, y, z);
  	printPos(current_pos);
  	
}

//-------------------------------------------------------------------
void enable_steppers()
{
  	digitalWrite(X_ENABLE_PIN, ENABLE_ON);
  	digitalWrite(Y_ENABLE_PIN, ENABLE_ON);
  	digitalWrite(Z_ENABLE_PIN, ENABLE_ON);
}

//-------------------------------------------------------------------
void enable_z()
{
  	digitalWrite(Z_ENABLE_PIN, ENABLE_ON);
}


//-------------------------------------------------------------------
void disable_steppers()
{
  	digitalWrite(X_ENABLE_PIN, !ENABLE_ON);
  	digitalWrite(Y_ENABLE_PIN, !ENABLE_ON);
}


//-------------------------------------------------------------------
void disable_z()
{
  	digitalWrite(Z_ENABLE_PIN, !ENABLE_ON);
}


//-------------------------------------------------------------------
void setMicroSteps(int _microSteps){
  	println(" microsteps are set by jumper");
  	// MICROSTEPS
  	//	MS1		MS2		MS3
  	//	0		0		0	1
  	//	1		0		0	1/2
  	//	0		1		0	1/4
  	//	1		1		0	1/8
  	//	1		1		1	1/16
  	
	/*
	 switch (_microSteps) {
	 case 1:
	 digitalWrite(MS1_PIN, LOW);
	 digitalWrite(MS2_PIN, LOW);
	 digitalWrite(MS3_PIN, LOW);
	 break;
	 case 2:
	 digitalWrite(MS1_PIN, HIGH);
	 digitalWrite(MS2_PIN, LOW);
	 digitalWrite(MS3_PIN, LOW);
	 break;
	 case 4:
	 digitalWrite(MS1_PIN, LOW);
	 digitalWrite(MS2_PIN, HIGH);
	 digitalWrite(MS3_PIN, LOW);
	 break;
	 case 8:
	 digitalWrite(MS1_PIN, HIGH);
	 digitalWrite(MS2_PIN, HIGH);
	 digitalWrite(MS3_PIN, LOW);
	 break;
	 case 16:
	 digitalWrite(MS1_PIN, HIGH);
	 digitalWrite(MS2_PIN, HIGH);
	 digitalWrite(MS3_PIN, HIGH);
	 break;
	 default:
	 print(_microSteps);
	 println(" is not a correct microstep setting");
	 return;
	 break;
	 }
	 microSteps = _microSteps;
	 */
}




