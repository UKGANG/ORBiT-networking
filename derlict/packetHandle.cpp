
#include "packetHandle.h"

int packetHandle::createPacket(string* outStr, string* inStr, int packetNum, int packetType)
{
	outStr->clear();
	int dataLength = inStr->length();
	outStr->append(1, (char)(dataLength >> 8));
	outStr->append(1, (char)dataLength);
	outStr->append(1, packetNum);
	outStr->append(1, packetType);
	outStr->append(*inStr);
	char checksum = 0;
	for(int i = 0; i < inStr->length(); i++) // compute sum8 checksum
		checksum += (*inStr)[i];
	outStr->append(1,checksum);
	outStr->append(1,(((dataLength >> 8) ^ dataLength) ^ packetNum) ^ packetType); //length checksum

	return(0);
}

int packetHandle::openPacket(string* outStr, string* inStr, int *packetNumber, int *packetType)
{
	int length = static_cast<unsigned char>((*inStr)[0]) * 0x100 + static_cast<unsigned char>((*inStr)[1]);

	if(length + 6 != inStr->length())
		return(-1);

	if((*inStr)[inStr->length()-1] != ((((*inStr)[0] ^ (*inStr)[1]) ^ (*inStr)[2]) ^ (*inStr)[3]))
		return(-2); // check if length (&packet num) checksum is in the right place

	if(packetNumber != nullptr)
		*packetNumber = (*inStr)[2];

	if(packetType != nullptr)
		*packetType = (*inStr)[4];

	*outStr = inStr->substr(4,inStr->length() - 6); //load the data into output

	int checksum = 0;
	for(int i = 4; i < inStr->length()-2 ; i++) // compute sum8 checksum
	{
		checksum += (*inStr)[i];
	}

	if((char)checksum != (*inStr)[inStr->length()-2])
		return(-3);
	return(length);
}
