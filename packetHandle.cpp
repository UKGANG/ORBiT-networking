
#include "packetHandle.h"

packetHandle::packetHandle(int buff_size)
{
	if(buff_size < 0)
	{
		packetBufferSize = 0;
		return;
	}
	else if(buff_size > 255)
	{
		packetBufferSize = 255;
	}
	
	packetBufferSize = buff_size;
	packetBuffer = new string[packetBufferSize]; // allocate buffer TODO add error checking
	curPacket = 0;
}

packetHandle::~packetHandle()
{
	if(packetBufferSize > 0)
		delete[] packetBuffer; // dealocate buffer
}

int packetHandle::createPacket(string* outStr, string* inStr)
{
	outStr->clear();
	int dataLength = inStr->length();
	outStr->append(1, (char)(dataLength >> 8));
	outStr->append(1, (char)dataLength);
	outStr->append(1, curPacket);
	outStr->append(*inStr);
	char checksum = 0;
	for(int i = 0; i < inStr->length(); i++) // compute sum8 checksum
		checksum += (*inStr)[i];
	outStr->append(1,checksum);
	outStr->append(1,((dataLength >> 8) ^ dataLength) ^ curPacket); //length checksum

	if(packetBufferSize > 0 && packetBuffer != nullptr)
	{
		packetBuffer[curPacket] = *inStr;
		curPacket++;
		if(curPacket >= packetBufferSize)
			curPacket = 0;
	}

	return(0);
}

int packetHandle::openPacket(string* outStr, string* inStr, int *packetNumber)
{
	int length = static_cast<unsigned char>((*inStr)[0]) * 0x100 + static_cast<unsigned char>((*inStr)[1]);

	if(length + 5 != inStr->length())
		return(-1);

	if((*inStr)[inStr->length()-1] != (((*inStr)[0] ^ (*inStr)[1]) ^ (*inStr)[2]))
		return(-2); // check if length (&packet num) checksum is in the right place

	if(packetNumber != nullptr)
		*packetNumber = (*inStr)[2];

	*outStr = inStr->substr(3,inStr->length() - 5); //load the data into output

	int checksum = 0;
	for(int i = 3; i < inStr->length()-2 ; i++) // compute sum8 checksum
	{
		checksum += (*inStr)[i];
	}

	if((char)checksum != (*inStr)[inStr->length()-2])
		return(-3);
	return(length);
}
