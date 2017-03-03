
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <stdio.h>

#include "serialio.h"
#include "xbeeHandle.h"
#include "packetHandle.h"

//#define dev (char*)"/dev/ttyUSB0"
//#define dev (char*)"/dev/ttyS0"
//#define dev (char*)"/dev/pts/25"
#define dev (char*)"/dev/pts/26"

#define packetSoftSizeLimit (unsigned int)512

using namespace std;

atomic<bool> run(true);

void inputHandle(serialIO* ser, mutex* mSerial, string* buffer) //input handle
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
			mSerial->lock();
			buffer->append(1,temp);
			mSerial->unlock();
		}
	}
	tcsetattr(0, TCSANOW, &initial_settings);
	run.store(false);
}

int main()
{
	serialIO ser;

	if(ser.initialize(dev, B9600, 0, false, 0) != 0)
		return(0);

	xbeeHandle xbee(&ser);
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


	cout << "Entered console mode!" << endl << '>';

	mutex mSerial;
	string userInput;
	string serialBuffer;
	unsigned int curSerLength = 0;
	thread cinThread(inputHandle, &ser, &mSerial, &userInput);

	packetHandle packHandle(5);

	while(run)
	{
		// user handle
		if(mSerial.try_lock())
		{
			if(userInput.length() > 0)
			{
				string writeString;
				if(userInput.length() > packetSoftSizeLimit)
				{
					writeString = userInput.substr(0, packetSoftSizeLimit);
					userInput.erase(0, packetSoftSizeLimit);
				}
				else
				{
					writeString = userInput;
					userInput.clear();
				}
				cout << writeString << flush;
				string packet;
				packHandle.createPacket(&packet , &writeString);
				ser.writeTo(packet);

			}
			mSerial.unlock();
		}


		// serial handle
		string tempBuf = ser.readFrom();
		if(tempBuf.length() > 0 || serialBuffer.length() >= 2)
		{
			serialBuffer += tempBuf;
			if(curSerLength == 0 && serialBuffer.length() >= 2)
			{
				curSerLength = static_cast<unsigned char>(serialBuffer[0])*0x100 + 
					       static_cast<unsigned char>(serialBuffer[1]);

				if(curSerLength > packetSoftSizeLimit) // soft limit
				{
					curSerLength = 0;
					serialBuffer.erase(0, 1); // remove 1 byte and try again
					//serialBuffer.clear();
				}
			}
			if(serialBuffer.length() >= (curSerLength + 3 + 2) && curSerLength > 0) 
			{	// +1 for checksum +2 for length
				// all data recived
				string packetString = serialBuffer.substr(0, curSerLength + 3 + 2);

				string data;
				int res = packHandle.openPacket(&data, &packetString);
			
				if(res >= 0)
				{
					cout << "Unpacked (" << res << "): " << data << endl;
					serialBuffer.erase(0, curSerLength + 3 + 2);
					curSerLength = 0;
				}
				else if(res == -3)
				{
					cout << "Erronious unpack (" << res << "):  " << data << endl;
					serialBuffer.erase(0, curSerLength + 3 + 2);
					curSerLength = 0;
				}
				else
				{
					cout << "Failed unpack! (" << res << ")" << endl;
					serialBuffer.erase(0, 1); // remove 1 byte and try again
					curSerLength = 0;
				}
			}
		}
	}

	cinThread.join();

	return(0);
}


