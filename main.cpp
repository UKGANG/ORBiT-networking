
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <stdio.h>

#include "serialio.h"
#include "xbeeHandle.h"
#include "transmitHandle.h"

#define dev (char*)"/dev/ttyUSB0"
//#define dev (char*)"/dev/ttyS0"
//#define dev (char*)"/dev/pts/24"
//#define dev (char*)"/dev/pts/2"

#define packetSoftSizeLimit (unsigned int)512
#define versionString "Communications Handle Ver: 0.12"


using namespace std;

atomic<bool> run(true);

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
	bool quietMode = false;
	bool consoleMode = true;
	bool setDestination = false;
	bool getDestination = false;
	bool getVoltage = false;
	bool setSource = false;
	bool getSource = false;
	int sourceAddr = 0;
	int destinationAddr = 0;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-q") == 0)
		{ // supress all messages
			quietMode = true;
		}
		else if(strcmp(argv[i], "-c") == 0)
		{ // turn off console mode
			consoleMode = false;
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
		else
		{
			cout << "Unknown argument: " << argv[i] << endl;
			return(1);
		}
	}

	if(quietMode == false)
		cout << versionString << endl;

	serialIO ser;

	if(ser.initialize(dev, B9600, 0, false, 0) != 0)
		return(0);

	xbeeHandle xbee(&ser);
	transmitHandle tHandle(&ser, packetSoftSizeLimit, 5);

	if(setDestination)
		xbee.setDestinationAddress(destinationAddr);
	if(getDestination)
		cout << "Destination Address: " << xbee.getDestinationAddress() << endl;
		
	if(setSource)
		xbee.setSourceAddress(sourceAddr);
	if(getSource)
		cout << "Source Address: " << xbee.getSourceAddress() << endl;
		
	if(getVoltage == true)
		cout << "Board Voltage: " << xbee.getBoardVoltage() << endl;


	/*cout << "Received Signal Strength: "<< xbee.getReceivedSignalStrength() << endl;

	cout << "baud rate: " << xbee.getInterfaceDataRate() << endl;
	cout << "changing serial baud rate..." << endl;
	xbee.setInterfaceDataRate(B19200);
	cout << "baud rate: " << xbee.getInterfaceDataRate() << endl;*/


	if(consoleMode == true)
	{
		if(quietMode == false)
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
					if(quietMode == false)
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
				else if(res == -3 && quietMode == false)
					cout << "Erronious recive (" << res << "): \"" << retVal << "\""<< endl;
				else if(res != -4 && quietMode == false)
					cout << "Failed recive! (" << res << ")" << endl;
			}
		}

		cinThread.join();
	}
	return(0);
}


