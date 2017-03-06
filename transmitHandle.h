#ifndef TRANSMIT_HANDLE
#define TRANSMIT_HANDLE

#include <iostream>
#include <string.h>


#include "serialio.h"

using namespace std;

class transmitHandle
{
public:

	transmitHandle(serialIO* serial, int packetSoftSizeLimit = 255 , int packetBufferSize = 0);
	~transmitHandle();

	int transmitData(string transmitString, int packetType = 0);
	int getRecivedData(string *returnData);

	int createPacket(string* outStr, string* inStr, int packetNum = 0, int packetType = 0);
	int openPacket(string* outStr, string* inStr, int *packetNumber = nullptr, int *packetType = nullptr);

	int getSoftLimit();

private:

	serialIO* serialHandle;
	string* packetBuffer;
	string* recivedPacketBuffer;

	unsigned int curPacket;
	unsigned int curRecivedPacket;
	unsigned int softSizeLimit;
	unsigned int curSerLength;
	unsigned int packBufSize;
	unsigned int recPackBufSize;

	unsigned int maxDiscrepency;

	string serialBuffer;
	

};

#endif
