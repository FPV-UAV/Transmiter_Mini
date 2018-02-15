/*
* Transmiter-Mini for RFduino / Simblee
*
* Reads every 5 min a Libre Freestyle sensor via NFC and transmit the BG readings to xDrip+ via BLE.
*
* Code used from the work of @keencave, @UPetersen, @MarekMacner, @jstevensog, @savek-cc and @bertrooode
*
* 2018 Marek Macner (@FPV-UAV)
*/


/* ------------------ hardware definition ----------- */
#define HW_Simblee
//#define HW_RFduino

#ifdef HW_Simblee
	#include <SimbleeBLE.h>
#endif
#ifdef HW_RFduino
	#include <RFduinoBLE.h>
#endif

#define PIN_SPI_SCK   4
#define PIN_SPI_MOSI  5
#define PIN_SPI_MISO  3
#define PIN_SPI_SS    6
#define PIN_IRQ       2 

#define NFCTIMEOUT 500

/* ------------------ libraries section ----------- */
#include <SPI.h>
#include <Stream.h>
#include <Memory.h>
#include <ota_bootloader.h>

/* ------------------ datatypes and structures ----------- */
typedef struct  __attribute__((packed))
{
	bool sensorDataOK;
	String sensorSN;
	byte   sensorStatusByte;
	String sensorStatusString;
	byte  nextTrend;
	byte  nextHistory;
	int minutesSinceStart;
	int minutesHistoryOffset;
	int trend[16];
	int history[32];
} SensorDataDataType;

typedef struct  __attribute__((packed))
{
	byte allBytes[345];
} AllBytesDataType;

typedef struct  __attribute__((packed))
{
	float voltage;
	float temperature;
} BatteryDataType;

typedef struct  __attribute__((packed))
{
	long voltage;
	int voltagePercent;
	double temperatureC;
	double temperatureF;
	double rssi;
} DeviceDataType;

#define  str(x)   xstr(x) 
#define  xstr(x)  #x      

/* ------------------ variables section ----------- */
bool debug = false;			// main switch for debugging via serial port

byte NFCReady = 0;
byte resultBuffer[400];
byte dataBuffer[400];


unsigned int time_loop_started = 0;
unsigned int time_elapsed = 0;
byte runPeriod = 1;

byte dtResultCode;
byte dtDeviceID[13];
byte dtRomCRC[2];

byte siUid[8];
byte siResultCode;
byte siResponseFlags;
byte siInfoFlags;
byte siErrorCode;
String siSensorSN;

byte sensorDataHeader[24];
byte sensorDataBody[296];
byte sensorDataFooter[24];
SensorDataDataType sensorData;
DeviceDataType devData;
bool BatteryOK = false;
bool SensorOK = false;
String TxBuffer1 = "";
int i = 0;
String msgLoop = "";

long cycleCount = 0;

byte sn[8];

void setup()
{
	Serial.begin(9600);
	print_state("Setup - start");
	print_state("wersja do dfu");
	bleSetup();
	nfcInit();
	sendNFC_ToHibernate();
	delay(10);

	if (debug) print_state("Setup - end");
	Serial.println("Start");
	if (!debug) Serial.end();
}

void loop()
{
	time_loop_started = millis();
	deviceData();

	NFC_wakeUP();
	checkSensor();

	if (readSensorData())
	{
		SensorOK = true;
		print_state("Sensor data OK");
		decodeSensor();
	}
	else
	{
		SensorOK = false;
		print_state("Sensor data Fail");

	}	
	sendNFC_ToHibernate();
	


	TxBuffer1 = "";
	TxBuffer1 += String(sensorData.trend[0] * 100);
	TxBuffer1 += " ";
	TxBuffer1 += String(devData.voltage);
	TxBuffer1 += " ";
	TxBuffer1 += String(devData.voltagePercent);
	TxBuffer1 += " ";
	TxBuffer1 += String((int)(sensorData.minutesSinceStart / 10));
	
	if (SensorOK)
	{
		print_state(TxBuffer1);
		SimbleeBLE.send(TxBuffer1.cstr(), TxBuffer1.length());
	}
	SensorOK = false;
	
	time_elapsed = millis() - time_loop_started;
	msgLoop = "Loop time: ";
	msgLoop += String(time_elapsed);
	msgLoop += " ms";
	print_state(msgLoop);
	#ifdef HW_RFDuino
		RFduino_ULPDelay((60000 * runPeriod) - time_elapsed);
	#endif
	#ifdef HW_Simblee
		Simblee_ULPDelay((60000 * runPeriod) - time_elapsed);
	#endif
		cycleCount++;
		if (cycleCount > 10) runPeriod = 5;

}
