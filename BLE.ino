void bleSetup()
{
	SimbleeBLE.deviceName = "LimiTTer";
	SimbleeBLE.advertisementData = "data";
	SimbleeBLE.customUUID = "c97433f0-be8f-4dc8-b6f0-5343e6100eb4";
	SimbleeBLE.advertisementInterval = MILLISECONDS(500);
	SimbleeBLE.txPowerLevel = +4;
	SimbleeBLE.begin();
}

void SimbleeBLE_onConnect()
{
	print_state("+++++++++++++ BLE conn");
}
void SimbleeBLE_onDisconnect()
{
	print_state("+++++++++++++ BLE dis conn");
}

void SimbleeBLE_onReceive(char *data, int len) 
{
	print_state("BLE - command received");
	if (data[0] == 'F')		// switch to firmware update mode
	{
		print_state("BLE - firmware update command received");
		delay(10000);
		SimbleeBLE.end();
		ota_bootloader_start();
		
	}
	else if (data[0] == '1') changeRunPeriod(1);
	else if (data[0] == '2') changeRunPeriod(2);
	else if (data[0] == '3') changeRunPeriod(3);
	else if (data[0] == '4') changeRunPeriod(4);
	else if (data[0] == '5') changeRunPeriod(5);
	else if (data[0] == '6') changeRunPeriod(6);
	else if (data[0] == '7') changeRunPeriod(7);
	else if (data[0] == '8') changeRunPeriod(8);
	else if (data[0] == '9') changeRunPeriod(9);

	else print_state("BLE - wrong data received");	
}
void changeRunPeriod(int i)
{
	String msg = "BLE -> New run period = ";
	msg += String(i);
	print_state(msg);
	runPeriod = i;
}