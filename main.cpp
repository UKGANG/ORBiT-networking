
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <stdio.h>

#include "serialio.h"
#include "xbeeHandle.h"
#include "transmitHandle.h"

//#define dev (char*)"/dev/ttyUSB0"
//#define dev (char*)"/dev/ttyS0"
#define dev (char*)"/dev/pts/24"
//#define dev (char*)"/dev/pts/2"

#define packetSoftSizeLimit (unsigned int)512
#define versionString "Communications Handle Ver: 0.1"


using namespace std;

atomic<bool> run(true);
bool quetMode = false;

void inputHandle(serialIO* ser, mutex* mBuffer, string* buffer) //input handle
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
		char temp = getchar();
		if(temp == 3)
			break;


		if(temp != -1)
		{
			mBuffer->lock();
			buffer->append(1,temp);
			mBuffer->unlock();
		}
	}
	tcsetattr(0, TCSANOW, &initial_settings);
	run.store(false);
}

int main(int argc, char* argv[])
{

	if(argc > 1)
	{
		if(strcmp(argv[1], "-q") == 0)
		{ // supress all messages
			quetMode = true;
		}
	}

	if(quetMode == false)
		cout << versionString << endl;

	serialIO ser;

	if(ser.initialize(dev, B9600, 0, false, 0) != 0)
		return(0);

	xbeeHandle xbee(&ser);
	transmitHandle tHandle(&ser, packetSoftSizeLimit, 5);

	/*cout << "Board Voltage: " << xbee.getBoardVoltage() << endl;
	cout << "Received Signal Strength: "<< xbee.getReceivedSignalStrength() << endl;

	cout << "setting destination address..." << endl;
	xbee.setDestinationAddress(0xffff);
	cout << "Destination Address: " << xbee.getDestinationAddress() << endl;

	cout << "setting source address..." << endl;
	xbee.setSourceAddress(0xffff);
	cout << "Source Address: " << xbee.getSourceAddress() << endl;

	cout << "baud rate: " << xbee.getInterfaceDataRate() << endl;
	cout << "changing serial baud rate..." << endl;
	xbee.setInterfaceDataRate(B19200);
	cout << "baud rate: " << xbee.getInterfaceDataRate() << endl;*/


	if(quetMode == false)
		cout << "Entered console mode: " << endl;

	mutex mBuffer;
	string userInput;
	thread cinThread(inputHandle, &ser, &mBuffer, &userInput);

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
				if(quetMode == false)
					cout << writeString << flush;
				tHandle.transmitData(writeString);

			}
			mBuffer.unlock();
		}

		// serial packet handle
		string retVal;
		int res = tHandle.getRecivedData(&retVal);
		if(res != 0)
		{
			if(res)
				cout << retVal << flush;
			else if(res == -3)
				cout << "Erronious recive (" << res << "): \"" << retVal << "\""<< endl;
			else if(res != -4)
				cout << "Failed recive! (" << res << ")" << endl;
		}
	}

	cinThread.join();

	return(0);
}


