
void decodeSensor()
{
	for (int i = 0; i < 24; i++) sensorDataHeader[i] = dataBuffer[i];
	for (int i = 24; i < 320; i++) sensorDataBody[i - 24] = dataBuffer[i];
	for (int i = 320; i < 344; i++) sensorDataFooter[i - 320] = dataBuffer[i];
	decodeSensorHeader();
	decodeSensorBody();
	decodeSensorFooter();
	if (debug) displaySensorData();
	for (int i = 0; i < 344; i++) dataBuffer[i] = 1;
}

void decodeSensorHeader()
{
	sensorData.sensorStatusByte = sensorDataHeader[4];
	switch (sensorData.sensorStatusByte)
	{
	case 0x01:
		sensorData.sensorStatusString = "not yet started";
		break;
	case 0x02:
		sensorData.sensorStatusString = "starting";
		break;
	case 0x03:
		sensorData.sensorStatusString = "ready";
		break;
	case 0x04:
		sensorData.sensorStatusString = "expired";
		break;
	case 0x05:
		sensorData.sensorStatusString = "shutdown";
		break;
	case 0x06:
		sensorData.sensorStatusString = "failure";
		break;
	default:
		sensorData.sensorStatusString = "unknown state";
		break;
	}
}

void decodeSensorBody()
{
	sensorData.nextTrend = sensorDataBody[2];
	sensorData.nextHistory = sensorDataBody[3];
	byte minut[2];
	minut[0] = sensorDataBody[293];
	minut[1] = sensorDataBody[292];
	sensorData.minutesSinceStart = minut[0] * 256 + minut[1];
	sensorData.minutesHistoryOffset = (sensorData.minutesSinceStart - 3) % 15 + 3;
	int index = 0;
	for (int i = 0; i < 16; i++)                                // 16 bloków co 1 minutê
	{
		index = 4 + (sensorData.nextTrend - 1 - i) * 6;
		if (index < 4) index = index + 96;
		byte pomiar[6];
		for (int k = index; k < index + 6; k++) pomiar[k - index] = sensorDataBody[k];
		sensorData.trend[i] = ((pomiar[1] << 8) & 0x0F00) + pomiar[0];
	}
	index = 0;
	for (int i = 0; i < 32; i++)                                // 32 bloki co 15 minut
	{
		index = 100 + (sensorData.nextHistory - 1 - i) * 6;
		if (index < 100) index = index + 192;
		byte pomiar[6];
		for (int k = index; k < index + 6; k++) pomiar[k - index] = sensorDataBody[k];
		sensorData.history[i] = ((pomiar[1] << 8) & 0x0F00) + pomiar[0];
	}
}

void decodeSensorFooter()
{

}

void displaySensorData()
{


	Serial.println("Sensor data OK.");
	Serial.print("Sensor s/n: ");
	Serial.println(sensorData.sensorSN);
	Serial.print("Sensor status: ");
	Serial.print(sensorData.sensorStatusByte, HEX);
	Serial.print(" - ");
	Serial.println(sensorData.sensorStatusString);
	Serial.print("Next trend position: ");
	Serial.println(sensorData.nextTrend);
	Serial.print("Next history position: ");
	Serial.println(sensorData.nextHistory);
	Serial.print("Minutes since sensor start: ");
	Serial.println(sensorData.minutesSinceStart);
	Serial.print("Minutes trend to history offset: ");
	Serial.println(sensorData.minutesHistoryOffset);
	Serial.print("BG trend: ");
	for (int i = 0; i<16; i++) Serial.printf(" %f", (float)(sensorData.trend[i] / 10.0f));
	Serial.println("");
	Serial.print("BG history: ");
	for (int i = 0; i<32; i++) Serial.printf(" %f", (float)(sensorData.history[i] / 10.0f));
	Serial.println("");	
}