#include "SD.h"

File writeFile;			// the file to write to.
long numbyteswritten;	// number of bytes written

//----------------------------------------------------------------------
void startWritingToFile( char * filename, int length){
    
    state = FILEWRITE; // set state to FILEWRITE so we don't interpret the parse commands

    if(root){
		root.close();
	}
    init_SD();
	root = SD.open("/");
	root.rewindDirectory();

	// start writing to a specific file
	filename[length] = '\0';
    // cleanup names
	for(int i = 0; i < length; i++){
        char c = filename[i];
        if((c >= 48 && c <= 57) || // numbers : 48 -> 57
           (c >= 65 && c <= 90) || // uppercase : 65 -> 90
           (c >= 97 && c <= 122)|| // lowercase : 97 -> 122
           (c == '.')
           ){
            continue;
        }else{
            filename[i]= '_';
        }
    }
    SerialUSB.print(" Writing to ");
    SerialUSB.println(filename);
	
    // close the write file if it's still open ( shouldn't happen )
	if(writeFile){
        SerialUSB.println(" WARNING : writefile was still open ");
		writeFile.close();
	}
    
	SD.remove(filename);
	writeFile = SD.open(filename, FILE_WRITE);
    if(!writeFile){
        SerialUSB.println(" ERROR couldn't open file to write ");
        return;
    }
	numbyteswritten = 0;
}


//----------------------------------------------------------------------
void stopWriting(){
    // if there is still something left in the buffer, write it to the card
	if(iSerialBuf > 0){
		writeFile.write((const uint8_t *) serialBuffer, iSerialBuf);
		numbyteswritten += iSerialBuf;
		iSerialBuf = 0;
        SerialUSB.println(" WARNING : Needed final write!! ");
	}
    
    // close the file.
	if(writeFile){
		writeFile.close();
	}
    if(root){
        root.close();
    }
    
	SerialUSB.print("Done writing ");
	SerialUSB.print(numbyteswritten);
	SerialUSB.println(" bytes to file");
    getFileList();
}


//----------------------------------------------------------------------
void writeFromSerial(){
	// write to the file
    
    iSerialBuf = 0; // should this be 0 ? yes: we always clean out the complete buffer in this function

    while (SerialUSB.available() > 0){
		
        char inByte = SerialUSB.read();
		
        if(inByte == '\r'){
            state = WORKING;
			stopWriting();
			return;
		}else{
			if(iSerialBuf < bufferLength - 1){
				serialBuffer[iSerialBuf] = inByte;
				iSerialBuf++;
			}else{
                // buffer is full, write it to the SD Card
				writeFile.write((const uint8_t *)serialBuffer, iSerialBuf);
				numbyteswritten += iSerialBuf;
				iSerialBuf = 0;
                
                // add the character to buffer for the next write!
                serialBuffer[iSerialBuf] = inByte;
				iSerialBuf++;
			}
		}
        
	}
    // end while loop
    
	if(iSerialBuf > 0){
		writeFile.write((const uint8_t *) serialBuffer, iSerialBuf);
		numbyteswritten += iSerialBuf;
		iSerialBuf = 0;
	}	
}




