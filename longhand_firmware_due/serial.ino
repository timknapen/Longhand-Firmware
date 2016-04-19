
//------------------------------------------------------------
void checkSerial(){
    while (SerialUSB.available() > 0){
		char inByte = SerialUSB.read();
        switch(inByte){
            case '\n':
            case '\r':
                serialBuffer[iSerialBuf] = '\0'; // add null terminator
                parseMessage(serialBuffer, iSerialBuf);
                iSerialBuf = 0;
                break;
            default:
                if( iSerialBuf < bufferLength-1 ){
                    serialBuffer[iSerialBuf] = inByte;
                    iSerialBuf++;
                }else{
                    println("ERROR : SERIAL BUFFER OVERFLOW!");
                    iSerialBuf = 0; // we'll just throw away what we had?
                }
                break;
        }
	}
}



