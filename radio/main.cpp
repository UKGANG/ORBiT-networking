#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <stdio.h>
#include <fstream>
#include <sstream>

#include "serialio.h"
#include "xbeeHandle.h"
#include "transmitHandle.h"
#include "logHandle.h"

//#define dev (char*)"/dev/pts/17"
//#define dev (char*)"/dev/pts/18"
//#define dev (char*)"/dev/ttyUSB0"
//#define dev (char*)"/dev/ttyS0"
#define dev (char*)"/dev/ttyS4"


#define packetSoftSizeLimit (unsigned int)512
#define versionString "Communications Handle Ver: 0.13"

// Radio buffers
#define FOREIGN_BUFFER 256
#define LOCAL_BUFFER 256

using namespace std;

atomic<bool> run(true);

// global log class (yes, I know, bad programming right there)
logHandle hLog;

void inputHandle(serialIO* ser, mutex* mBuffer, string* buffer, bool filterMode = false) //input handle
{
	struct termios initial_settings,new_settings;

	tcgetattr(0,&initial_settings);

	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &new_settings);

	while(run)
	{
		int temp = getchar();

		//if(temp != EOF)
		//	cout << (int)temp;

		if(filterMode) // filter characters to process locally only
		{
			if(temp == 3)
				break;
		}

		if(temp != EOF) // check if character was sent at all (not EOF)
		{
			mBuffer->lock();
			buffer->append(1,(char)temp);
			mBuffer->unlock();
		}
	}
	tcsetattr(0, TCSANOW, &initial_settings);
	run.store(false);
}

int main(int argc, char* argv[])
{
	bool quietMode = false;
	bool consoleMode = false;
	bool filterMode = true;
	bool getVoltage = false;

	bool setDestination = false;
	bool getDestination = false;


	bool setSource = false;
	bool getSource = false;

	bool tryReset = false;

	bool setSerialRate = false;
	bool setInitialRate = false;

	bool setRFRate = false;
	bool getRFRate = false;


	int sourceAddr = 0;
	int destinationAddr = 0;

	int serialRate = B9600;
	int intitialRate = B9600;

	int RFRate = 0;

	string devPath = dev;
	string logPath = "./radio.log";

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-q") == 0)
		{ // supress all messages
			quietMode = true;
		}
		else if(strcmp(argv[i], "-c") == 0)
		{ // turn on console mode
			consoleMode = true;
		}
		else if(strcmp(argv[i], "-f") == 0)
		{ // turn off filter mode
			filterMode = false;
		}
		else if(strcmp(argv[i], "-d") == 0)
		{ // set device
			if(!(i+1 < argc))
			{
				cout << "Error: -d missing argument!" << endl;
				return(1);
			}
			devPath = argv[i+1];
			i++; //increment i due to argument
		}
		else if(strcmp(argv[i], "-l") == 0)
		{ // set logfile
			if(!(i+1 < argc))
			{
				cout << "Error: -l missing argument!" << endl;
				return(1);
			}
			logPath = argv[i+1];
			i++; //increment i due to argument
		}
		else if(strcmp(argv[i], "--set-destination") == 0)
		{  // set destination
			if(!(i+1 < argc))
			{
				cout << "Error: set-destination missing argument!" << endl;
				return(1);
			}
			try
			{
			    destinationAddr = stoi(argv[i+1]);
			}
			catch (std::exception const &e)
			{
				cout << "Error: set-destination only accepts a number!" << endl;
				return(1);
			}

			i++; //increment i due to argument
			setDestination = true;
		}
		else if(strcmp(argv[i], "--get-destination") == 0)
		{  // get destination
			getDestination = true;
		}
		else if(strcmp(argv[i], "--set-source") == 0)
		{  // set source
			if(!(i+1 < argc))
			{
				cout << "Error: set-source missing argument!" << endl;
				return(1);
			}
			try
			{
			    sourceAddr = stoi(argv[i+1]);
			}
			catch (std::exception const &e)
			{
				cout << "Error: set-source only accepts a number!" << endl;
				return(1);
			}

			i++; //increment i due to argument
			setSource = true;
		}
		else if(strcmp(argv[i], "--get-source") == 0)
		{  // get source
			getSource = true;
		}
		else if(strcmp(argv[i], "--get-voltage") == 0)
		{  // get board voltage
			getVoltage = true;
		}
		else if(strcmp(argv[i], "--try-reset") == 0)
		{ // try resetting board with text commands
			tryReset = true;
		}
		else if(strcmp(argv[i], "--set-serial-baud") == 0)
		{ // set serial baud rate
			if(!(i+1 < argc))
			{
				cout << "Error: set-serial-baud missing argument!" << endl;
				return(1);
			}
			try
			{
			    serialRate = stoi(argv[i+1]);
			}
			catch (std::exception const &e)
			{
				cout << "Error: set-serial-baud only accepts a number!" << endl;
				return(1);
			}

			i++; //increment i due to argument
			setSerialRate = true;
		}
		else if(strcmp(argv[i], "--serial-baud") == 0)
		{ // set serial baud rate
			if(!(i+1 < argc))
			{
				cout << "Error: serial-baud missing argument!" << endl;
				return(1);
			}
			try
			{
			    intitialRate = stoi(argv[i+1]);
			}
			catch (std::exception const &e)
			{
				cout << "Error: initial-baud only accepts a number!" << endl;
				return(1);
			}

			i++; //increment i due to argument
			setInitialRate = true;
		}
		else if(strcmp(argv[i], "--set-rf-rate") == 0)
		{ // set set rf rate
			if(!(i+1 < argc))
			{
				cout << "Error: set-rf-rate missing argument!" << endl;
				return(1);
			}
			try
			{
				RFRate = stoi(argv[i+1]);
			}
			catch (std::exception const &e)
			{
				cout << "Error: set-rf-rate only accepts a number!" << endl;
				return(1);
			}

			i++; //increment i due to argument
			setRFRate = true;
		}
		else if(strcmp(argv[i], "--get-rf-rate") == 0)
		{ // try resetting board with text commands
			getRFRate = true;
		}
		else
		{
			cout << "Unknown argument: " << argv[i] << endl;
			return(1);
		}
	}

	hLog.setLogFile(logPath);

	if(quietMode == false)
		cout << versionString << endl;
	hLog << versionString << endl;

	serialIO ser;

	if(setInitialRate == true)
	{
		int rateParameter = serialIO::intToBaud(intitialRate);
		if(rateParameter == -1)
		{
			cout << "Not a valid baud rate!" << endl;
			return(-1);
		}

		if(ser.initialize(devPath, rateParameter, 0, false, 0) != 0)
			return(-1);
	}
	else
	{
		if(ser.initialize(devPath, B9600, 0, false, 0) != 0)
			return(-1);
	}

	xbeeHandle xbee(&ser);
	transmitHandle tHandle(&ser, packetSoftSizeLimit, LOCAL_BUFFER, FOREIGN_BUFFER);


	if(tryReset) // TODO make this work
	{
		cout << "Trying to reset..." << endl;
		int res = xbee.tryReset();
		cout << "Reset result: " << res << endl;
	}

	if(setDestination)
		xbee.setDestinationAddress(destinationAddr);

	if(getDestination)
		cout << "Destination Address: " << xbee.getDestinationAddress() << endl;

	if(setSource)
		xbee.setSourceAddress(sourceAddr);

	if(getSource)
		cout << "Source Address: " << xbee.getSourceAddress() << endl;

	if(getVoltage == true)
		cout << "Board Voltage: " << xbee.getBoardVoltage() << "V" << endl;

	if(setRFRate)
		xbee.setRFDataRate(RFRate);

	if(getRFRate)
		cout << "RF rate: " << xbee.getRFDataRate() << endl;


	if(setSerialRate)
	{
		int rateParameter = serialIO::intToBaud(serialRate);
		if(rateParameter == -1)
		{
			cout << "Not a valid baud rate!" << endl;
			return(-1); // baud rate not standard
		}

		cout << "changing serial baud rate..." << endl;
		if(xbee.setInterfaceDataRate(rateParameter) != 0)
		{
			cout << "Error setting serial baud rate!" << endl;
			return(-1);
		}
	}

	//cout << "Received Signal Strength: "<< xbee.getReceivedSignalStrength() << endl;

	if(consoleMode == true)
	{
		if(quietMode == false)
		{
			cout << "Entered console mode:\n"
			     << "CAUTION: EXPERIMENTAL!" << endl;
		}

		mutex mBuffer;
		string userInput;
		userInput.clear();
		thread cinThread(inputHandle, &ser, &mBuffer, &userInput, filterMode);

		while(run)
		{
			// user handle
			if(mBuffer.try_lock())
			{
				if(userInput.length() > 0)
				{
					string writeString;
					if(userInput.length() > tHandle.getSoftLimit())
					{
						writeString = userInput.substr(0, packetSoftSizeLimit);
						userInput.erase(0, packetSoftSizeLimit);
					}
					else
					{
						writeString = userInput;
						userInput.clear();
					}
					if(quietMode == false)
					{
						cout << writeString << flush;
					}
					tHandle.transmitData(writeString);

				}
				mBuffer.unlock();
			}

			// serial packet handle
			string retVal;
			int res = tHandle.getRecivedData(&retVal);
			if(res != 0)
			{
				if(res >= 0)
				{
					//writeTeeLog(retVal, quietMode);
					cout << retVal << flush;
				}
				else if(res == -3)
				{
					if(!quietMode)
						cout << "Erronious recive (" << res << "): \"" << retVal << "\""<< endl;
					hLog << "Erronious recive (" << res << "): \"" << retVal << "\""<< endl;
				}
				else if(res == -4)
				{
					//if(!quietMode)
					//	cout << "Missed packet!"<< endl;
					hLog << "Missed packet!"<< endl;
				}
				else if(quietMode == false)
				{
					if(!quietMode)
						cout << "Failed recive! (" << res << ")" << endl;
					hLog << "Failed recive! (" << res << ")" << endl;
				}
			}
		}

		run.store(false);

		cinThread.join();
	}
	return(0);
}
