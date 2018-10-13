
#include <string>
#include <fstream>
#include <unordered_map>

#include "include/json.hpp"
#include "socket.h"
#include "dataProtocol.h"

#define MULT_DEST

enum class logStates {All, DefOnly, FileOnly};

struct SensorConfig
{
	//data defined in this struct to be used only to validate incoming packets
	//do not use to populate missing fields in packets
	int id;
	std::string desc;
	std::string unit;
	int relayRate;
	int lastRelay;
	std::string* relayDestinationArr;
	int relayDestinationLength;
	std::string logFile;
};

class sensorHandle
{
public:

	sensorHandle();
	~sensorHandle();

	int initFromConfig(std::string configFile, bool printSettings);
	int initFromString(std::string jsonString, bool printSettings);

	int processPacket(std::string packet, udpSocketHandle* sendConn);
	

private:
	SensorConfig* sensorsArr;
	int numSensors;

	logStates logSetting;
	std::string logRoot;
	std::string logSubFolder;
	std::string UndefLogFile;

	int defaultRelayRate;
	std::string* defaultRelayDestArr;
	int defaultRelayDestLength;
	std::string defaultLogFile;
	std::string* UnknownDestArr;
	int UnknownDestLength;

	std::unordered_map<std::string, std::ofstream*> fileMap; // <filename, file>


	dataProtocol dataProt;

	int initFromJson(nlohmann::json configJson, bool printSettings);
	int createLogs();

	int writeToLog(int sensorID, std::string data);
	int relayPacket(int sensorID, std::string packet, udpSocketHandle* sendConn, bool overideRate);

	void printLogSettings();
	void printDefaultSettings();

	void deleteSensorsArray();


};
