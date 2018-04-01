
#include "transmitHandle.h"

transmitHandle::transmitHandle(serialIO* serial, int packetSoftSizeLimit, int packetBufferSize)
{
	serialHandle = serial;
	softSizeLimit = packetSoftSizeLimit;
	curSerLength = 0;

	packBufSize = packetBufferSize;
	recPackBufSize = packetBufferSize; // incase future request

	packetBuffer = new string[packetBufferSize]; // allocate buffer TODO add error checking
	recivedPacketBuffer = new string[packetBufferSize]; // TODO see previous

	curPacket = 0;
	curRecivedPacket = 0;
}

transmitHandle::~transmitHandle()
{
	if(packBufSize > 0)
		delete[] packetBuffer; // dealocate buffer

	if(recPackBufSize > 0)
		delete[] recivedPacketBuffer; // dealocate buffer
}

int transmitHandle::transmitData(string transmitString, int packetType)
{
	if(transmitString.length() > softSizeLimit)
		return(-1);

	string packet;
	int res = createPacket(&packet , &transmitString, curPacket, packetType);
	if(res < 0)
		return(res);

	if(packBufSize > 0 && packetBuffer != nullptr)
	{
		packetBuffer[curPacket] = transmitString;
		curPacket++;
		if(curPacket >= packBufSize)
			curPacket = 0;
	}

	return(serialHandle->writeTo(packet));
}

int transmitHandle::getRecivedData(string *returnData)
{
	string tempBuf = serialHandle->readFrom();
	if(tempBuf.length() > 0 || serialBuffer.length() >= 2)
	{
		serialBuffer += tempBuf;
		if(curSerLength == 0 && serialBuffer.length() >= 2)
		{
			curSerLength = static_cast<unsigned char>(serialBuffer[0])*0x100 + 
				       static_cast<unsigned char>(serialBuffer[1]);

			if(curSerLength > softSizeLimit) // soft limit
			{
				curSerLength = 0;
				serialBuffer.erase(0, 1); // remove 1 byte and try again
				return(-4);
				//serialBuffer.clear();
			}
		}
		if(serialBuffer.length() >= (curSerLength + 6) && curSerLength > 0) 
		{	// +1 for checksum +2 for length
			// all data recived
			string packetString = serialBuffer.substr(0, curSerLength + 6);

			string data;
			int packetNumber;
			int packetType;
			int res = openPacket(&data, &packetString, &packetNumber, &packetType);

			if(res >= 0) // unpack success
			{
				if(packetType == 0) // data
				{
					string transmitString; 
					transmitString.append(1,packetNumber);// acknowledge recived package
					transmitString.append(1,6);
					string packet;
					createPacket(&packet , &transmitString, curPacket, 1);
					serialHandle->writeTo(packet);
					//TODO return value

					serialBuffer.erase(0, curSerLength + 6);
					curSerLength = 0;
					*returnData = data;
					return(res);
				}
				else if(packetType == 1) // acknowledge
				{
					/*if(data[1] != 6) //check if data arrived correctly
					{
						//resend if not
						string packet;
						createPacket(&packet , &(packetBuffer[data[0]]), data[0], 0);
						serialHandle->writeTo(packet);
					}*/ // add order handle
					serialBuffer.erase(0, curSerLength + 6);
					curSerLength = 0;
					return(0);
				}
				else
				{
					serialBuffer.erase(0, curSerLength + 6);
					curSerLength = 0;
					*returnData = data;
					return(res);
				}


				/*if(recPackBufSize > 0 && recivedPacketBuffer != nullptr)
				{
					recivedPacketBuffer[curRecivedPacket] = data;
					curRecivedPacket++;
					if(curRecivedPacket >= recPackBufSize)
						curRecivedPacket = 0;
				}*/
			}
			else if(res == -3) // unpack, partial success
			{
				if(packetType == 0)
				{
					string transmitString;
					transmitString.append(1,packetNumber);// acknowledge recived package
					transmitString.append(1,21);
					string packet;
					createPacket(&packet , &transmitString, curPacket, 1);
					serialHandle->writeTo(packet);
					//TODO return value
				}
				/*else if(packetType == 1)
				{
					// acknowlege with wrong data
					// do nothing
				}*/

				serialBuffer.erase(0, curSerLength + 6);
				*returnData = data;
				curSerLength = 0;
			}
			else // unpack fail
			{
				serialBuffer.erase(0, 1); // remove 1 byte and try again
				curSerLength = 0;
			}
			return(res);
		}
	}
	return(0);
}

int transmitHandle::createPacket(string* outStr, string* inStr, int packetNum, int packetType)
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

int transmitHandle::openPacket(string* outStr, string* inStr, int *packetNumber, int *packetType)
{
	int length = static_cast<unsigned char>((*inStr)[0]) * 0x100 + static_cast<unsigned char>((*inStr)[1]);

	if(length + 6 != inStr->length())
		return(-1);

	if((*inStr)[inStr->length()-1] != ((((*inStr)[0] ^ (*inStr)[1]) ^ (*inStr)[2]) ^ (*inStr)[3]))
		return(-2); // check if length (&packet num) checksum is in the right place

	if(packetNumber != nullptr)
		*packetNumber = (*inStr)[2];

	if(packetType != nullptr)
		*packetType = (*inStr)[3];

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

int transmitHandle::getSoftLimit()
{
	return(softSizeLimit);
}
