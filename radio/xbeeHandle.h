#ifndef XHANDLE
#define XHANDLE

#include <string.h>

#include "serialio.h"

using namespace std;

class xbeeHandle
{
public:

	xbeeHandle(serialIO* serial);

	int tryReset();

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

	// rf interface commands
	int setRFDataRate(unsigned int rate);
	int getRFDataRate();

private:

	serialIO* serialHandle;
	string binaryCommand(string cmdStr, int reciveLength);


};

#endif
