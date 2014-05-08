
//--------------------------------------------------------------
void bezier(long x0, long y0, long x1, long y1, long x2, long y2, long x3, long y3){
	// from OpenFrameworks' bezier interpolation
	
	long lastx, lasty, llastx, llasty;
	llastx = lastx = x0;
	llasty = lasty = y0;
	
	float   ax, bx, cx;
	float   ay, by, cy;
	float   t, t2, t3 = 0;
	long     x = 0, y = 0;
	
	
	// polynomial coefficients
	cx = 3.0f * (x1 - x0);
	bx = 3.0f * (x2 - x1) - cx;
	ax = x3 - x0 - cx - bx;
	cy = 3.0f * (y1 - y0);
	by = 3.0f * (y2 - y1) - cy;
	ay = y3 - y0 - cy - by;
	
	int skipped = 0;
	t = 0;
	float bezierStep = 1.0f / bezierResolution;
	t = 0;
	moveTo(x0, y0, 0); // put the pen down.
	while ( t <= ( 1.0f - bezierStep ) ) {
		
		t += bezierStep;
		t2 = t * t;
		t3 = t2 * t;
		
		x = (int)((ax * t3) + (bx * t2) + (cx * t) + x0);
		y = (int)((ay * t3) + (by * t2) + (cy * t) + y0);
		
		if( ((x-lastx)*(x-lastx) + (y-lasty)*(y-lasty)) >= 1 ){
			/* with speed up and down */
			set_target(floor(x), floor(y), 0);
			dda_move(min_delay);
			
			llastx = lastx;
			llasty = lasty;
			lastx = x;
			lasty = y;
		}
		else {
			skipped ++;
		}
	} // end while
	
	if( ((x3-lastx)*(x3-lastx) + (y3-lasty)*(y3-lasty)) >= 1 ){
		set_target(floor(x3), floor(y3), 0);
		dda_move(max_delay);
	}
}



//--------------------------------------------------------------
void ellipse(long cx, long cy, long rx, long ry){
	set_target( cx + rx*cos(PI),
				cy + ry*sin(PI),
				0); // pen down!
	dda_move(min_delay);
	
	for(float i = 0; i <= 360.0f; i +=circleRes){
		set_target(	cx + rx*cos(PI + PI*(float)i/180.0),
					cy + ry*sin(PI + PI*(float)i/180.0));
		dda_move(min_delay);
	}
	set_target( cx + rx*cos(PI + PI*360.0f/180.0),
				cy + ry*sin(PI + PI*360.0f/180.0));
	dda_move(max_delay);//slow down for the last part
	
}



//--------------------------------------------------------------
void arc(float xc, float yc, float r, float beginAngle, float endAngle){
	// xc,yc = circle center
	// r = radius
	// beginAngle = begin angle 1 in degrees!
	// adif = angle dif to second point in degrees
	
	float adif = endAngle - beginAngle;
	// convert to RADIANS
	beginAngle = PI * beginAngle / 180.0f;
	adif = PI * adif / 180.0f;
	
	// calculate the angle / step:
	int numberOfSteps = (r * adif) / 4; // we'll make lines of 4 steps..
	float stepAngle =  adif / numberOfSteps;
	
	// rotMatrix
	// [ a b ]
	// [ c d ]
	// r_T = [ cos(phi) -sin(phi) ]
	//       [ sin(phi)  cos(phi) ] * r
	// applying rotation:
	//  x = x1 * Math.Cos(angleStep) - y1 * Math.Sin(angleStep);
	//  y = y1 * Math.Cos(angleStep) + x1 * Math.Sin(angleStep);
	float sinAngle, cosAngle;
	
	// the point we will be rotating:
	float x,y;
	
	// rotate to first position:
	cosAngle = cos(beginAngle);
	sinAngle = sin(beginAngle);
	// setup the first position
	x = r * cosAngle - 0 * sinAngle;
	y = 0 * cosAngle + r * sinAngle;
	
	// setup the step rotation matrix
	cosAngle = cos(stepAngle);
	sinAngle = sin(stepAngle);
	
	int counter = 0;
	float newx, newy;
	// put the pen down!
	moveTo( xc + x, yc + y, 0);
	
	for(int i = 0; i < numberOfSteps+1; i++){
		counter ++;
		if(counter < 20){
			newx =  x * cosAngle - y * sinAngle;
			newy =  y * cosAngle + x * sinAngle;
		}
		else{
			x = r;
			y = 0;
			float newCos = cos( beginAngle + i * stepAngle);
			float newSin = sin( beginAngle + i * stepAngle);
			newx =  x * newCos - y * newSin;
			newy =  y * newCos + x * newSin;
			counter = 0;
		}
		x = newx;
		y = newy;
		moveTo( xc + x, yc + y, 0);
	}
	
}
