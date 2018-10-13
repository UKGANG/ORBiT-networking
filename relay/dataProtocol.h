#ifndef DATA_PROTOCOL
#define DATA_PROTOCOL

#include <string>
#include <unordered_map>
#include <iostream>

struct dataPacket
{
	unsigned id;
	std::string data;
	std::string time;

	void print()
	{
		std::cout << "ID: " << id << " data(" << data.length() << "): " << data 
			  << " time(" << time.length() << "): " << time << std::endl;
	}
};

struct dataIdentifier
{
	unsigned id;
	std::string description;
	std::string units;

	void print()
	{
		std::cout << "ID: " << id << " Description(" << description.length() << "): " << description 
			  << " Units(" << units.length() << "): " << units << std::endl;
	}
};

class dataProtocol
{
public:

	~dataProtocol();

	// call this function for every packet, before any other operations on a packet
	int addToMemoryIfIdent(std::string packet);
	
	int removeIDFromMemory(int id);

	dataIdentifier* getIdentifierPtr(int id);

	// data packets
	int package(dataPacket packet, std::string* output);
	int unPackage(std::string packet, dataPacket* output);

	// identifier packets
	int package(dataIdentifier packet, std::string* output);
	int unPackageIdent(std::string packet, dataIdentifier* output);

private:

	 std::unordered_map<int, dataIdentifier*> identifierMap;
};

#endif
