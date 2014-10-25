#include <SD.h>
#include "LongPoint.h"
#include "pins.h"

File root;

//----------------------------------------------------------------------
void init_SD(){
  	pinMode(CSPIN, OUTPUT);
  	if (!SD.begin(CSPIN)) {
  		//println(" SD initialization failed!");
  		return;
  	}
}

//----------------------------------------------------------------------
void getFileList(){ //print out all the .LHD files on the SD card
  	init_SD();
  	root = SD.open("/");
  	root.rewindDirectory();
  	println("Files:");
  	while(true) {
  		File entry =  root.openNextFile();
  		if (!entry) {
  			break;
  		}
  		if (!entry.isDirectory()) {
  			// do some more checking
  			char* filename = entry.name();
  			if(filename[0] == '~' || filename[0] == '.' || filename[0] == '_'){
  				entry.close();
  				continue;
  			}
  			if(!hasExtension(entry, ".LHD")){
  				entry.close();
  				continue;
  			}
  			// if we are here, we have a good file, so print it
  			print("_f");
  			println(entry.name());
			delayMicroseconds(5);
  		}
  		entry.close();
  	}
  	if(root){
  		root.close();
  	}
}

//----------------------------------------------------------------------
void deleteFile(char * filename, int length){
	
	init_SD();
  	root = SD.open("/");
  	root.rewindDirectory();
  	
  	if(SD.remove(filename)){
		print("DELETED ");
		println(filename);
	}else{
		print("FAILED to delete ");
		println(filename);
	}
  	if(root){
  		root.close();
  	}
}

//----------------------------------------------------------------------
void drawFromSD(char * filename, int length){ // start the draw from SD
  	// make sure the filename is null terminated:
  	filename[length] = '\0';
  	init_SD();
  	
  	File file = SD.open(filename);
  	
  	if(file){
  		print("Starting to parse ");
  		println(file.name());
  		
		isDrawingFromFile = true;
		travelDistance = 0L;
  		parseFileContents(file);
  		isDrawingFromFile = false;
		
  		println(" --- done parsing (disable steppers)");
		if(bPreview){
			println(" --- travel distance : ");
			println(travelDistance);
		}
  		println(MACHINE_STOPPED); // c0 = ended drawing, a command the controller software understands
  		file.close();
  		disable_steppers();
  	}else{
  		print("I couldn't find the file ");
  		println(filename);
  	}
}

//----------------------------------------------------------------------
bool hasExtension(File file, char* ext){
  	//we only support extensions that are 3 chars long (for now) : a dot '.' plus the name
  	//first find length of filename
  	int namelen = 0;
  	
  	while(true){
  		char c = file.name()[namelen];
  		if(c == '\0') break;
  		if(namelen > 13){ // should NEVER happen, is just a safety
  			println("ERROR the filename is too long, should only be 13 chars long (see SD.h)");
  			break;
  		}
  		namelen++;
  	}
  	
  	if(namelen < 4) return false; // 3 chars and the "." before it = 4
  	// now check it..;
  	for(int i = 0; i < 4; i++){
  		if(ext[4-1-i] != file.name()[namelen-1-i]) return false;
  	}
  	return true;
}



//----------------------------------------------------------------------
void parseFileContents(File file){
  	
  	print("\tParsing file: ");
  	println(file.name());
  	println();
  	char buf[bufferLength];
  	int i = 0;
  	
  	// remember where the pen was in the real world
  	LongPoint memPos;
  	if(bPreview){
  		memPos.x = current_pos.x;
  		memPos.y = current_pos.y;
  		memPos.z = current_pos.z;
  	}
	offSet.x = current_pos.x;
	offSet.y = current_pos.y;
  	
  	
  	if(file){
  		while(file.available()){
  			// check serial to see if I have to stop
  			if(SerialUSB.available() > 0) {
  				println("\n Drawing stopped by user");
  				disable_steppers();
  				break; // break out of while loop?
  			}
  			char c =  file.read();
  			if( i < bufferLength){
  				switch (c){
  					case '\n':
  						buf[i] = '\0';  // end string
  						parseMessage(buf, i);
  						i = 0;
  						break;
  						// commands to ignore:
  					case 'w': // write to file
  					case 'W':
  					case 'p': // draw from file
  					case 'P':
  					case 'f': // print file list
  					case 'F':
  					case '?': // get info
  					case 't': // set preview mode
  					case 'T':
  					case 'd': // set verbose
  					case 'D':
  						break;
  					default:
  						buf[i] = c;
  						i++;
  						break;
  				}
  			}
  			else{
  				println("ERROR buffer is full! (BAD READ)");
  				i = 0;
  				break;
  			}
  		}
  		//goHome();
		// lift pen
		moveTo(current_pos.x, current_pos.y, 100); // brush up at current position
		moveTo(offSet.x, offSet.y);
  		if(bPreview){
  			// set the pen back to where it was.
  			set_position(memPos.x, memPos.y, memPos.z);
  		}
		offSet.x = 0;
		offSet.y = 0;
  	}else{
  		println("parseFileContents got passed a bad file");
  	}
}
