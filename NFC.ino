void configSPI()
{
	print_state("configSPI - set pinMode and paras");
	pinMode(PIN_SPI_SS, OUTPUT);
	pinMode(PIN_IRQ, OUTPUT);
	SPI.begin();
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);
	SPI.setFrequency(2000);
	print_state("configSPI - done");
}

void nfcInit()
{
	print_state("nfc_init - start");
	configSPI();
	NFC_wakeUP();
	NFCReady = 0;
	if (!checkSensor()) print_state("No sensor in range");
	print_state("nfc_init - done");
}

bool checkSensor()
{
	SetNFCprotocolCommand(true);
	if (Inventory_Command())
	{
		print_state("Sensor OK");
		
		for (int i = 0; i < 8; i++) sn[i] = resultBuffer[11 - i];
		print_state(decodeSN(sn));
//		getSysInfo();
	}
	else print_state("Missing sensor");
}

void sendNFC_ToHibernate()
{
	print_state("sendNFC-ToHibernate - start");
	const int length = 16;
	byte command[length];
	command[0] = 0x07;
	command[1] = 0x0E;
	command[2] = 0x08;
	command[3] = 0x04;
	command[4] = 0x00;
	command[5] = 0x04;
	command[6] = 0x00;
	command[7] = 0x18;
	command[8] = 0x00;
	command[9] = 0x00;
	command[10] = 0x00;
	command[11] = 0x00;
	command[12] = 0x00;
	command[13] = 0x00;
	command[14] = 0x00;
	command[15] = 0x00;
	send_NFC_Command(command, sizeof(command));
	print_state("sendNFC-ToHibernate - done");
}

void NFC_wakeUP()
{
	print_state("NFC_wakeUp - start");
	digitalWrite(PIN_IRQ, HIGH);
	delay(10);
	digitalWrite(PIN_IRQ, LOW);
	delayMicroseconds(100);
	digitalWrite(PIN_IRQ, HIGH);
	delay(10);
	print_state("NFC_wakeUp - done");
}

void send_NFC_Command(byte *commandArray, int length)
{
	digitalWrite(PIN_SPI_SS, LOW);
	SPI.transfer(0x00);
	for (int i = 0; i < length; i++) SPI.transfer(commandArray[i]);
	digitalWrite(PIN_SPI_SS, HIGH);
	delay(1);
}

void send_NFC_PollReceive(byte *command, int commandLength)
{	
	send_NFC_Command(command, commandLength);
	poll_NFC_UntilResponsIsReady();
	receive_NFC_Response();
}

void poll_NFC_UntilResponsIsReady()
{
	String msg = "";
	unsigned long ms = millis();
	byte rb;
	digitalWrite(PIN_SPI_SS, LOW);
	while ((resultBuffer[0] != 8) && ((millis() - ms) < NFCTIMEOUT))
	{
		rb = resultBuffer[0] = SPI.transfer(0x03);
		resultBuffer[0] = resultBuffer[0] & 0x08;
	}
	digitalWrite(PIN_SPI_SS, HIGH);
	delay(1);
	if (millis() - ms > NFCTIMEOUT) 
	{
		msg = "\r\n *** NFC response poll timeout *** -> response ";
		msg += String(rb);
		print_state(msg);
	}
}

void receive_NFC_Response()
{
	digitalWrite(PIN_SPI_SS, LOW);
	SPI.transfer(0x02);
	resultBuffer[0] = SPI.transfer(0);
	resultBuffer[1] = SPI.transfer(0);
	for (byte i = 0; i < resultBuffer[1]; i++) resultBuffer[i + 2] = SPI.transfer(0);
	digitalWrite(PIN_SPI_SS, HIGH);
	delay(1);
}

void SetNFCprotocolCommand(bool typ)
{
	print_state("SetNFCprotocolCommand - start");
	String msg = "";
	for (int t = 0; t < 9; t++)
	{		
		const int length = 4;
		byte command[length];
		command[0] = 0x02;
		command[1] = 0x02;
		command[2] = 0x01;
		if (typ) command[3] = 0x0D;
		else command[3] = 0x0F;
		send_NFC_PollReceive(command, sizeof(command));
		msg += "resultBuffer: ";
		for (byte i = 0; i < 2; i++) 
		{
			msg += String(resultBuffer[i], HEX);
			msg += " ";
		}
		if ((resultBuffer[0] == 0) & (resultBuffer[1] == 0)) {
			msg += ", Try=";
			msg += String(t);
			msg += " - PROTOCOL SET - OK";
			NFCReady = 1;
			break;
		}
		else {
			msg += ", Try=";
			msg += String(t);
			msg += " - BAD RESPONSE TO SET PROTOCOL";
			NFCReady = 0;
		}		
	}
	print_state(msg);
	print_state("SetNFCprotocolCommand - done");
}

bool Inventory_Command()
{
	String msg = "";
	print_state("Inventory - start");
	byte i = 0;
	digitalWrite(PIN_SPI_SS, LOW);
	SPI.transfer(0x00);
	SPI.transfer(0x04);
	SPI.transfer(0x03);
	SPI.transfer(0x26);
	SPI.transfer(0x01);
	SPI.transfer(0x00);
	digitalWrite(PIN_SPI_SS, HIGH);
	digitalWrite(PIN_SPI_SS, LOW);
	while (resultBuffer[0] != 8)
	{
		resultBuffer[0] = SPI.transfer(0x03);
		resultBuffer[0] = resultBuffer[0] & 0x08;
	}
	digitalWrite(PIN_SPI_SS, HIGH);
	digitalWrite(PIN_SPI_SS, LOW);
	SPI.transfer(0x02);
	resultBuffer[0] = SPI.transfer(0);
	resultBuffer[1] = SPI.transfer(0);
	for (i = 0; i<resultBuffer[1]; i++)
		resultBuffer[i + 2] = SPI.transfer(0);
	digitalWrite(PIN_SPI_SS, HIGH);
	msg = "Inventory - done => ";
	msg += String(resultBuffer[0], HEX);
	print_state(msg);
	if (resultBuffer[0] == 0x87) return false;
	if ((resultBuffer[0] == 0x80) || (resultBuffer[0] == 0x8E))
	{
		return true;
	}
	else
	{
		TxBuffer1 = "000999 00";
		TxBuffer1 += String(resultBuffer[0],HEX);
		TxBuffer1 += " 01 1";
		print_state(TxBuffer1);
		SimbleeBLE.send(TxBuffer1.cstr(), TxBuffer1.length());

		return false;
	}
	
}

bool readSensorData()
{
	byte resultCode = 0;
	int trials = 0;
	int maxTrials = 10;
	String msg = "";
	print_state("readSensorData() - ");
	SetNFCprotocolCommand(false);
	clearBuffer(dataBuffer);
	bool single = true;
	if (single)
	{
		for (int i = 0; i < 43; i++)
		{
			resultCode = ReadSingleBlockReturn(i);
			
			if (resultCode != 0x80 && trials < maxTrials) {
				i--;
				trials++;
			}
			else if (trials >= maxTrials) {
				break;
			}
			else {
				trials = 0;
				for (int j = 3; j < resultBuffer[1] + 3 - 4; j++) {
					dataBuffer[i * 8 + j - 3] = resultBuffer[j];
				}
			}
		}
	}
	else
	{
		resultCode = ReadMultipleBlockReturn(0, 4 );
		for (int j = 3; j < 42*6 + 3 - 4; j++) 
		{
			dataBuffer[i * 8 + j - 3] = resultBuffer[j];
		}
	}	
	bool resultH = checkCRC16(dataBuffer, 0);
	bool resultB = checkCRC16(dataBuffer, 1);
	bool resultF = checkCRC16(dataBuffer, 2);
	bool crcResult = false;

	if (resultH && resultB && resultF) crcResult = true;
	else crcResult = false;

	if (crcResult) NFCReady = 2;
	else NFCReady = 1;
	return crcResult;
}

byte ReadSingleBlockReturn(int blockNum)
{
	const int length = 6;					
	byte command[length];
	command[0] = 0x04;
	command[1] = 0x04;
	command[2] = 0x03;
	command[3] = 0x23;  	
	command[4] = blockNum;
	command[5] = 0x01;
	send_NFC_Command(command, length);
	poll_NFC_UntilResponsIsReady();
	receive_NFC_Response();

	return resultBuffer[0];
}

void clearBuffer(byte *tmpBuffer)
{
	for (int i = 0; i < sizeof(tmpBuffer); i++) tmpBuffer[i] = 0;
}

byte ReadMultipleBlockReturn(byte blockNum, byte blockCount)
{
	const int length = 6;
	byte command[length];
	command[0] = 0x04;
	command[1] = 0x04;
	command[2] = 0x03;
	command[3] = 0x23;
	command[4] = blockNum;
	command[5] = blockCount;
	send_NFC_Command(command, length);
	poll_NFC_UntilResponsIsReady();	
	receive_NFC_Response();
	Serial.print("RR ");
	Serial.print(resultBuffer[0], HEX);
	Serial.print(" - ");
	Serial.print(resultBuffer[1]);
	Serial.print(" - ");
	Serial.print(resultBuffer[2]);
	Serial.print(" - ");
	Serial.print(resultBuffer[3]);
	Serial.print(" - ");
	Serial.print(resultBuffer[4]);
	Serial.print(" - ");
	Serial.println(resultBuffer[5]);
	return resultBuffer[0];
}

byte getSysInfo()
{
	const int length = 4;
	byte command[length];
	command[0] = 0x04;
	command[1] = 0x02;
	command[2] = 0x02;
	command[3] = 0x2B;

	send_NFC_Command(command, length);
	poll_NFC_UntilResponsIsReady();
	receive_NFC_Response();

	Serial.print("SYS ");
	Serial.print(resultBuffer[0], HEX);
	Serial.print(" - ");
	Serial.print(resultBuffer[1]);
	Serial.print(" - ");
	Serial.print(resultBuffer[2]);
	Serial.print(" - ");
	Serial.print(resultBuffer[3], BIN);
	Serial.print(" - ");
	Serial.print(resultBuffer[4]);
	Serial.print(" - ");
	Serial.print(resultBuffer[5]);
	Serial.print(" - ");
	Serial.print(resultBuffer[6]);
	Serial.print(" - ");
	Serial.print(resultBuffer[7]);
	Serial.print(" - ");
	Serial.print(resultBuffer[8]);
	Serial.print(" - ");
	Serial.print(resultBuffer[9]);
	Serial.print(" - ");
	Serial.print(resultBuffer[10]);
	Serial.print(" - ");
	Serial.print(resultBuffer[11]);
	Serial.print(" - ");
	Serial.print(resultBuffer[12]);
	Serial.print(" - ");
	Serial.print(resultBuffer[13],HEX);
	Serial.print(" - ");
	Serial.print(resultBuffer[14],HEX);
	Serial.print(" - ");
	Serial.print(resultBuffer[15],HEX);
	Serial.print(" - ");
	Serial.println(resultBuffer[16]);


	Serial.print("SN ");
	Serial.print(sn[0]);
	Serial.print(" - ");
	Serial.print(sn[1]);
	Serial.print(" - ");
	Serial.print(sn[2]);
	Serial.print(" - ");
	Serial.print(sn[3]);
	Serial.print(" - ");
	Serial.print(sn[4]);
	Serial.print(" - ");
	Serial.print(sn[5]);
	Serial.print(" - ");
	Serial.print(sn[6]);
	Serial.print(" - ");
	Serial.println(sn[7]);
	return resultBuffer[0];
}

