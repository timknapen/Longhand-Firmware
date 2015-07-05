#ifndef _LongPoint_h_
#define _LongPoint_h_

struct LongPoint
{
	long x;
	long y;
	long z;
};

struct FloatPoint
{
	float x;
	float y;
	
	// don't bother with the Z axis for calculations
	FloatPoint normalized(){
		float l = sqrt(x*x + y*y);
		if( l == 0){
			return FloatPoint{0,0};
		}
		
		return FloatPoint{ x/l, y/l };
	}
	
	void set(float nx, float ny){
		x = nx;
		y = ny;
	}
	
	FloatPoint operator+(FloatPoint p){
		return FloatPoint{ p.x + x, p.y + y};
	}
	
	FloatPoint operator-(FloatPoint p){
		return FloatPoint{ x - p.x, y - p.y };
	}
	
	FloatPoint operator*(float t){
		return FloatPoint{ x * t, y * t };
	}
	
	FloatPoint getRotatedRad( float angle){
		// angle in radians
		float sina = sin( angle );
		float cosa = cos( angle );
		
		return FloatPoint{
			x * cosa - y * sina,
			x * sina + y * cosa
		};
		
	}
	
};


#endif
