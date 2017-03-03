#ifndef XHANDLE
#define XHANDLE

#include <string.h>
#include <cstdlib>
#include <stdlib.h>
#include <cstdio>

#include <termios.h> // for B-constant definitions

#include <iostream>

#include "serialio.h"

using namespace std;

class xbeeHandle
{
public:

	xbeeHandle(serialIO* serial);

	//diognostic commands
	float getBoardVoltage();
	int getReceivedSignalStrength();
	int getDestinationAddress();
	int getSourceAddress();

	int setDestinationAddress(unsigned int address);
	int setSourceAddress(unsigned int address);

	//serial interfacing commands
	int setInterfaceDataRate(int rate);
	int getInterfaceDataRate();

private:

	serialIO* serialHandle;
	string binaryCommand(string cmdStr, int reciveLength);
	

};

#endif
