// Longhand Firmware
//------------------------------------------------------------
void checkSerial(){
	
	char inByte;
	boolean endOfMessage = false;
	while (SerialUSB.available() > 0 && iSerialBuf < serialBufferLength && !endOfMessage){
		inByte = SerialUSB.read();
		serialBuffer[iSerialBuf] = inByte;
		iSerialBuf++;
		if(inByte == '\n'){ // end of a command
			endOfMessage = true;
			serialBuffer[iSerialBuf] = '\0';
			iSerialBuf = 0;
		}
	}
	if(endOfMessage){
		int i =0;
		while (((inByte = serialBuffer[i]) != '\0') && iNum < bufferLength){
			i++;
			
			switch(inByte){
				case endline:
					messageBuffer[iNum] = '\0';// add null terminator
					parseMessage(messageBuffer, iNum);
					iNum =0;
					break;
				default:
					messageBuffer[iNum] = inByte;
					iNum++;
					break;
			}
		}
		disable_steppers();
		
	}
}





