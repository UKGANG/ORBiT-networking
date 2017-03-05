#ifndef PACKET_HANDLE
#define PACKET_HANDLE

#include <iostream>
#include <string.h>

using namespace std;

class packetHandle
{
public:
	int createPacket(string* outStr, string* inStr, int packetNum = 0, int packetType = 0);
	int openPacket(string* outStr, string* inStr, int *packetNumber = nullptr, int *packetType = nullptr);

private:

};

#endif
