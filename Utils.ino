
void print_state(String str)				// print timestamp and description of current status/action
{
	if (debug) Serial.print(timeStamp());
	if (debug) Serial.println(str);
}


String timeStamp()
{
	String ts = "";
	unsigned long ms = millis();
	unsigned long val;
	int i;
	ts += "[";
	ts += String(ms / 60000);
	ts += "][";
	if ((val = ((ms / 1000) % 60)) < 10) ts += "0";
	ts += String(val);
	ts += "][";
	if ((val = (ms % 1000)) < 10) ts += "00";
	else if (val < 100) ts += "0";
	ts += String(val);
	ts += "] ";
	return ts;
}

String decodeSN(byte *data)
{
	byte uuid[8];
	String lookupTable[32] =
	{
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"A", "C", "D", "E", "F", "G", "H", "J", "K", "L",
		"M", "N", "P", "Q", "R", "T", "U", "V", "W", "X",
		"Y", "Z"
	};
	byte uuidShort[8];
	for (int i = 2; i < 8; i++)  uuidShort[i - 2] = data[i];
	uuidShort[6] = 0x00;
	uuidShort[7] = 0x00;
	String binary = "";
	String binS = "";
	for (int i = 0; i < 8; i++)
	{
		binS = String(uuidShort[i], BIN);
		int l = binS.length();
		if (l == 1) binS = "0000000" + binS;
		else if (l == 6) binS = "00" + binS;
		else if (l == 7) binS = "0" + binS;
		binary += binS;
	}
	String v = "0";
	char pozS[5];
	for (int i = 0; i < 10; i++)
	{
		for (int k = 0; k < 5; k++) pozS[k] = binary[(5 * i) + k];
		int value = (pozS[0] - '0') * 16 + (pozS[1] - '0') * 8 + (pozS[2] - '0') * 4 + (pozS[3] - '0') * 2 + (pozS[4] - '0') * 1;
		v += lookupTable[value];
	}
	sensorData.sensorSN = v;
	return v;
}

uint16_t crc16table[256] =
{ 0, 4489, 8978, 12955, 17956, 22445, 25910, 29887, 35912, 40385,
44890, 48851, 51820, 56293, 59774, 63735, 4225, 264, 13203, 8730,
22181, 18220, 30135, 25662, 40137, 36160, 49115, 44626, 56045, 52068,
63999, 59510, 8450, 12427, 528, 5017, 26406, 30383, 17460, 21949,
44362, 48323, 36440, 40913, 60270, 64231, 51324, 55797, 12675, 8202,
4753, 792, 30631, 26158, 21685, 17724, 48587, 44098, 40665, 36688,
64495, 60006, 55549, 51572, 16900, 21389, 24854, 28831, 1056, 5545,
10034, 14011, 52812, 57285, 60766, 64727, 34920, 39393, 43898, 47859,
21125, 17164, 29079, 24606, 5281, 1320, 14259, 9786, 57037, 53060,
64991, 60502, 39145, 35168, 48123, 43634, 25350, 29327, 16404, 20893,
9506, 13483, 1584, 6073, 61262, 65223, 52316, 56789, 43370, 47331,
35448, 39921, 29575, 25102, 20629, 16668, 13731, 9258, 5809, 1848,
65487, 60998, 56541, 52564, 47595, 43106, 39673, 35696, 33800, 38273,
42778, 46739, 49708, 54181, 57662, 61623, 2112, 6601, 11090, 15067,
20068, 24557, 28022, 31999, 38025, 34048, 47003, 42514, 53933, 49956,
61887, 57398, 6337, 2376, 15315, 10842, 24293, 20332, 32247, 27774,
42250, 46211, 34328, 38801, 58158, 62119, 49212, 53685, 10562, 14539,
2640, 7129, 28518, 32495, 19572, 24061, 46475, 41986, 38553, 34576,
62383, 57894, 53437, 49460, 14787, 10314, 6865, 2904, 32743, 28270,
23797, 19836, 50700, 55173, 58654, 62615, 32808, 37281, 41786, 45747,
19012, 23501, 26966, 30943, 3168, 7657, 12146, 16123, 54925, 50948,
62879, 58390, 37033, 33056, 46011, 41522, 23237, 19276, 31191, 26718,
7393, 3432, 16371, 11898, 59150, 63111, 50204, 54677, 41258, 45219,
33336, 37809, 27462, 31439, 18516, 23005, 11618, 15595, 3696, 8185,
63375, 58886, 54429, 50452, 45483, 40994, 37561, 33584, 31687, 27214,
22741, 18780, 15843, 11370, 7921, 3960
};

uint16_t computeCRC16(void *bytes, byte type)
{
	int number_of_bytes_to_read = 0;
	int offset = 0;
	if (type == 0)
	{
		number_of_bytes_to_read = 24;
		offset = 0;
	}
	else if (type == 1)
	{
		number_of_bytes_to_read = 296;
		offset = 24;
	}
	else if (type == 2)
	{
		number_of_bytes_to_read = 24;
		offset = 320;
	}
	byte *data = (byte*)bytes;
	uint16_t crc = 0xffff;
	for (int i = offset + 2; i < number_of_bytes_to_read + offset; ++i)           // first two bytes = crc16 included in data
	{
		crc = (uint16_t)((crc >> 8) ^ crc16table[(crc ^ data[i]) & 0xff]);
	}
	uint16_t reverseCrc = 0;
	for (int i = 0; i < 16; i++)
	{
		reverseCrc = (uint16_t)((uint16_t)(reverseCrc << 1) | (uint16_t)(crc & 1));
		crc >>= 1;
	}
	return reverseCrc;
}

bool checkCRC16(void *bytes, byte type)
{
	int number_of_bytes_to_read = 0;
	int offset = 0;
	if (type == 0)
	{
		number_of_bytes_to_read = 24;
		offset = 0;
	}
	else if (type == 1)
	{
		number_of_bytes_to_read = 296;
		offset = 24;
	}
	else if (type == 2)
	{
		number_of_bytes_to_read = 24;
		offset = 320;
	}

	byte *data = (byte*)bytes;
	uint16_t x = data[0 + offset] | (data[1 + offset] << 8);
	uint16_t crc16calculated = computeCRC16(bytes, type);
	if (crc16calculated == x) return true;
	else return false;
}

void deviceData()
{
	String msg = "";
	analogReference(VBG);
	int sensorValue = analogRead(1);
	devData.voltage = sensorValue * (380 / 1023.0) * 10;
	devData.voltagePercent = map(devData.voltage, 2200, 3000, 0, 100);
	if (devData.voltage < 1000)
	{
		BatteryOK = false;
		TxBuffer1 = "000999 0001 99 1";
		print_state(TxBuffer1);
		SimbleeBLE.send(TxBuffer1.cstr(), TxBuffer1.length());
	}
	else BatteryOK = true;
	print_state("Device data:");
	msg = " - Voltage [mv]: ";
	msg += String(devData.voltage);
	print_state(msg);
	msg = " - Voltage [%]: ";
	msg += String(devData.voltagePercent);
	print_state(msg);
	print_state(msg);

	
}



