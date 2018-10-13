
#include "dataProtocol.h"

#include <utility>
#include <cstring>

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;


dataProtocol::~dataProtocol()
{
	 for ( unordered_map<int, dataIdentifier*>::iterator it = identifierMap.begin();
		it != identifierMap.end();
		it++ )
	{
		delete it->second; // remove all identifications before removing map
	}
}

int dataProtocol::addToMemoryIfIdent(string packet)
{

	dataIdentifier* tempData = new dataIdentifier;// = new dataIdentifier;
	int res = unPackageIdent(packet, tempData);
	if(res != 0)
	{
		delete tempData;
		return(res);
	}

	pair<int, dataIdentifier*> tempPair (tempData->id ,tempData); // commit to map
	if(identifierMap.insert(tempPair).second == false)
		return(-3);

	return(0);
}

int dataProtocol::removeIDFromMemory(int id)
{
	dataIdentifier* temp = getIdentifierPtr(id);
	if(temp == nullptr)
		return(0);
	delete temp;
	return(identifierMap.erase(id));
}

dataIdentifier* dataProtocol::getIdentifierPtr(int id)
{
	if(identifierMap.count(id) != 1)
		return(nullptr);

	return(identifierMap[id]);
}


int dataProtocol::unPackage(string packet, dataPacket* output)
{
	uint packid = ((uchar)packet[0] << 24) + ((uchar)packet[1] << 16) + ((uchar)packet[2] << 8) + (uchar)packet[3];

	if(packid == 0)
		return(-1); // is an identification packet

	// is a data packet

	uint dataLength = ((uchar)packet[4] << 24) + ((uchar)packet[5] << 16) + 
			 ((uchar)packet[6] << 8) + (uchar)packet[7];

	uint timeLength = ((uchar)packet[8] << 24) + ((uchar)packet[9] << 16) + 
			 ((uchar)packet[10] << 8) + (uchar)packet[11];

	if(packet.length() < 12 + dataLength + timeLength)
		return(-1); // invalid packet

	output->id = packid;

	for(int i = 0; i < dataLength; i++)
	{
		output->data.push_back(packet[i + 12]);
	}

	for(int i = 0; i < timeLength; i++)
	{
		output->time.push_back(packet[i + dataLength + 12]);
	}

	return(0);
}

int dataProtocol::unPackageIdent(string packet, dataIdentifier* output)
{
	uint packid = ((uchar)packet[0] << 24) + ((uchar)packet[1] << 16) + ((uchar)packet[2] << 8) + (uchar)packet[3];

	if(packid != 0) // not an identification packet
		return(1);

	// is an identification packet

	uint descLength = ((uchar)packet[4] << 24) + ((uchar)packet[5] << 16) + 
			 ((uchar)packet[6] << 8) + (uchar)packet[7];

	uint unitLength = ((uchar)packet[8] << 24) + ((uchar)packet[9] << 16) + 
			 ((uchar)packet[10] << 8) + (uchar)packet[11];

	if(packet.length() < 12 + descLength + unitLength)
		return(-1); // invalid packet

	if(descLength < 4)
		return(-1); // invalid packet

	// unpack packet
	uint id = ((uchar)packet[12] << 24) + ((uchar)packet[13] << 16) + ((uchar)packet[14] << 8) + (uchar)packet[15];

	if(id == 0) // id cannot be 0
		return(-2);

	output->id = id;

	for(int i = 4; i < descLength; i++)
	{
		output->description.push_back(packet[i + 12]);
	}

	for(int i = 0; i < unitLength; i++)
	{
		output->units.push_back(packet[i + 12 + descLength]);
	}

	//packet is valid
	return(0);
}

int dataProtocol::package(dataPacket packet, string* output)
{
	output->clear();
	output->push_back((packet.id & 0xff000000) >> 24);
	output->push_back((packet.id & 0xff0000) >> 16);
	output->push_back((packet.id & 0xff00) >> 8);
	output->push_back(packet.id & 0xff);

	output->push_back((packet.data.length() & 0xff000000) >> 24);
	output->push_back((packet.data.length() & 0xff0000) >> 16);
	output->push_back((packet.data.length() & 0xff00) >> 8);
	output->push_back(packet.data.length() & 0xff);

	output->push_back((packet.time.length() & 0xff000000) >> 24);
	output->push_back((packet.time.length() & 0xff0000) >> 16);
	output->push_back((packet.time.length() & 0xff00) >> 8);
	output->push_back(packet.time.length() & 0xff);

	output->append(packet.data);
	output->append(packet.time);
	return(0);
}

int dataProtocol::package(dataIdentifier packet, std::string* output)
{
	output->clear();
	output->push_back(0);
	output->push_back(0);
	output->push_back(0);
	output->push_back(0);

	output->push_back((packet.description.length() + 4 & 0xff000000) >> 24);
	output->push_back((packet.description.length() + 4 & 0xff0000) >> 16);
	output->push_back((packet.description.length() + 4 & 0xff00) >> 8);
	output->push_back(packet.description.length() + 4 & 0xff);

	output->push_back((packet.units.length() & 0xff000000) >> 24);
	output->push_back((packet.units.length() & 0xff0000) >> 16);
	output->push_back((packet.units.length() & 0xff00) >> 8);
	output->push_back(packet.units.length() & 0xff);

	output->push_back((packet.id & 0xff000000) >> 24);
	output->push_back((packet.id & 0xff0000) >> 16);
	output->push_back((packet.id & 0xff00) >> 8);
	output->push_back(packet.id & 0xff);

	output->append(packet.description);
	output->append(packet.units);
	return(0);
}

