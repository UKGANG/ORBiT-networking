#ifndef PACKET_HANDLE
#define PACKET_HANDLE

#include <iostream>
#include <string.h>

using namespace std;

class packetHandle
{
public:

	packetHandle(int buff_size);
	~packetHandle();
	int createPacket(string* outStr, string* inStr);
	int openPacket(string* outStr, string* inStr, int *packetNumber = nullptr);

private:

	int packetBufferSize;
	string* packetBuffer;

	int curPacket;
};

#endif
