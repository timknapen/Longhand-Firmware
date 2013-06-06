// Longhand Firmware

#include "SD.h"

File writeFile;			// the file to write to.
long numbyteswritten;	// number of bytes written

//----------------------------------------------------------------------
void startWritingToFile( char * filename, int length){
	// start writing to a specific file
	filename[length] = '\0';
	
	// delete the existing printjob if it exists
	if(writeFile){
		writeFile.close();
	}
	SD.remove(filename);
	writeFile = SD.open(filename, FILE_WRITE);
	numbyteswritten = 0;
}


//----------------------------------------------------------------------
void stopWriting(){
	// close the file.
	
	if(writeFile){
		writeFile.close();
	}
	SerialUSB.print("Done writing ");
	SerialUSB.print(numbyteswritten);
	SerialUSB.println(" bytes to file");
}


//----------------------------------------------------------------------
void writeFromSerial(){
	// write to the file
	char inByte;
	int i = 0;
	while (SerialUSB.available() > 0){
		inByte = SerialUSB.read();
		if(inByte == '\r'){
			state = WORKING;
			stopWriting();
			return;
		}
		else{
			if(i < bufferLength - 1){
				serialBuffer[i] = inByte;
				i++;
			}else{
				writeFile.write((const uint8_t *)serialBuffer, i);
				numbyteswritten += i;
				i =0;
			}
		}
	}
	if(i > 0){
		writeFile.write((const uint8_t *) serialBuffer, i);
		numbyteswritten += i;
		i = 0;
	}	
}




