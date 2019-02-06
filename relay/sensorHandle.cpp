

#include "sensorHandle.h"
#include "jsonParse.h"
#include "include/json.hpp"

#include <iostream>
#include <ctime>
#include <sys/stat.h>

using namespace std;
using json = nlohmann::json;

sensorHandle::sensorHandle()
{
	sensorsArr = nullptr;
	numSensors = 0;
	defaultRelayDestLength = 0;
	UnknownDestLength = 0;
	isInitialized = false;
}

sensorHandle::~sensorHandle()
{
	if(isInitialized == false)
		return;

	// make sure to clear all dynamic memory
	deleteSensorsArray();

	if (defaultRelayDestArr != nullptr)
		delete[] defaultRelayDestArr;

	if (UnknownDestArr != nullptr)
		delete[] UnknownDestArr;


	 for ( unordered_map<string, ofstream*>::iterator it = fileMap.begin();
		it != fileMap.end();
		it++ )
	{
		it->second->close();
		delete it->second; // remove all identifications before removing map
	}

	numSensors = 0;
	defaultRelayDestLength = 0;
	UnknownDestLength = 0;
	isInitialized = false;

}

int sensorHandle::initFromConfig(std::string configFile, bool printSettings)
{
	try
	{
		json configJson = jsonParse::parseFile(configFile);
		int res = initFromJson(configJson, printSettings);

		if(res == 0)
			isInitialized = true;
		return(res);
	}
	catch (...)
	{
		cerr << "Error while parsing configuration file!" << endl;
		return(-1);
	}

}

int sensorHandle::initFromString(std::string jsonString, bool printSettings)
{
	try
	{
		int res = initFromJson(json::parse(jsonString), printSettings);

		if(res == 0)
			isInitialized = true;
		return(res);
	}
	catch (...)
	{
		cerr << "Error while parsing configuration string!" << endl;
		return(-1);
	}
}


int sensorHandle::processPacket(string packet, udpSocketHandle* sendConn)
{
	int res = dataProt.addToMemoryIfIdent(packet);
	if(res == 1)
	{
		dataPacket tempData;
		res = dataProt.unPackage(packet, &tempData);
		if(res == 0)
		{
			dataIdentifier* identifierPtr = dataProt.getIdentifierPtr(tempData.id);
			if(identifierPtr != nullptr)
			{
				cout << "Recived data: " << tempData.data << " " << identifierPtr->units
				     << " Measured at time: " << tempData.time << endl;

				string dataString = to_string(tempData.id) + '\t' + tempData.data + ' ' +
				                    identifierPtr->units + "\tAt T: " + tempData.time + 's';

				writeToLog(tempData.id, dataString);
				relayPacket(tempData.id, packet, sendConn, false);
			}
			else
			{
				cout << "Error: Recived data without identification! (ID: "
				     << tempData.id << ")" << endl;

				string dataString = to_string(tempData.id) + '\t' + tempData.data +
				                    " NO IDENT\tAt T: " + tempData.time + 's';

				writeToLog(tempData.id, dataString);
				relayPacket(tempData.id, packet, sendConn, false);
			}
		}
		else
		{
			cout << "Error: Malformed package recived!(" << res << ")" << endl;
		}
	}
	else if(res == 0)
	{
		dataIdentifier tempIdent;
		dataProt.unPackageIdent(packet, &tempIdent);

		cout << "Recived Ident: " << tempIdent.id
		     << " Desc: " << tempIdent.description
		     << " Units: " << tempIdent.units << endl;

		relayPacket(tempIdent.id, packet, sendConn, true);
	}
	else if(res == -3)
	{
		dataIdentifier tempIdent;
		dataProt.unPackageIdent(packet, &tempIdent);

		cout << "Re-recived ident (" << tempIdent.id << ")!" << endl;
	}
	else if(res != 0)
	{
		cout << "Error: Malformed package recived! (" << res << ")" << endl;
	}
	return(0);
}

int sensorHandle::initFromJson(json configJson, bool printSettings)
{

	time_t now = time(0);
	tm *lTime = localtime(&now);

	// Log settings

	// default log settings
	logSetting = logStates::All;
	logRoot = "./Logs";
	logSubFolder = "/LOG:%F_%X";
	UndefLogFile = "UndefLog";

	// load settings, if exist
	if(!configJson["Log"].is_null())
	{
		if(configJson["Log"]["LogLevel"].is_string())
		{
			if(configJson["Log"]["LogLevel"] == "All")
			{
				logSetting = logStates::All;
			}
			else if(configJson["Log"]["LogLevel"] == "DefOnly")
			{
				logSetting = logStates::DefOnly;
			}
			else if(configJson["Log"]["LogLevel"] == "FileOnly")
			{
				logSetting = logStates::FileOnly;
			}
		}
		if(configJson["Log"]["LogRoot"].is_string())
		{
			logRoot = configJson["Log"]["LogRoot"];
		}
		if(configJson["Log"]["LogSubfolder"].is_string())
		{
			logSubFolder = configJson["Log"]["LogSubfolder"];
		}
		if(configJson["Log"]["UndefLog"].is_string())
		{
			UndefLogFile = configJson["Log"]["UndefLog"];
		}
	}

	// take care of any special parameters in folder names
	char logSubFolderBuff[255];
	strftime (logSubFolderBuff,sizeof(logSubFolderBuff),logSubFolder.c_str(),lTime);

	logSubFolder.clear();
	logSubFolder = logSubFolderBuff;
	// remove all special characters for compatibility (mainly windows)
	replace(logSubFolder.begin(), logSubFolder.end(), ':', '_');

	// Default Values

	// set default values
	defaultRelayRate = 0; // no input is sent out
	defaultRelayDestArr = nullptr; // no destination if not defined
	defaultLogFile = "DefLog"; // default log file, if not defined
	UnknownDestArr = nullptr; // no destination if not defined

	// load settings, if exist
	if(!configJson["Defaults"].is_null())
	{
		if(configJson["Defaults"]["RelayRate"].is_number())
		{
			defaultRelayRate = configJson["Defaults"]["RelayRate"];
		}
		if(configJson["Defaults"]["RelayDest"].is_array())
		{
			// allocate memory
			defaultRelayDestLength = configJson["Defaults"]["RelayDest"].size();
			if(defaultRelayDestLength != 0)
			{
				defaultRelayDestArr = new string[defaultRelayDestLength];

				int i = 0;
				for (json::iterator it = configJson["Defaults"]["RelayDest"].begin(); 
				     it != configJson["Defaults"]["RelayDest"].end(); ++it)
				{
					if(!(*it).is_string())
					{
						//must be string, if not error
						delete[] defaultRelayDestArr;
						defaultRelayDestArr = nullptr;
						return(-1);
					}
					defaultRelayDestArr[i] = (*it);
					i++;
				}
			}
			else
			{
				defaultRelayDestArr = nullptr;
			}
		}
		if(configJson["Defaults"]["LogFile"].is_string())
		{
			defaultLogFile = configJson["Defaults"]["LogFile"];
		}
		if(configJson["Defaults"]["UnknownDest"].is_array())
		{
			// allocate memory
			UnknownDestLength = configJson["Defaults"]["UnknownDest"].size();
			if(UnknownDestLength != 0)
			{
				UnknownDestArr = new string[UnknownDestLength];

				int i = 0;
				for (json::iterator it = configJson["Defaults"]["UnknownDest"].begin(); 
				     it != configJson["Defaults"]["UnknownDest"].end(); ++it)
				{
					if(!(*it).is_string())
					{
						//must be string, if not error
						delete[] UnknownDestArr;
						UnknownDestArr = nullptr;
						return(-1);
					}
					UnknownDestArr[i] = (*it);
					i++;
				}
			}
			else
			{
				UnknownDestArr = nullptr;
			}
		}
	}


	// load sensors

	// return if no sensors are defined
	if(!configJson["Sensors"].is_array())
	{
		return(0);
	}

	if(numSensors != 0)
		return(-1); // sensors already populated

	int i = 0;
	// allocate required memory
	numSensors = configJson["Sensors"].size();

	if(logSetting == logStates::All)
	{
		if(UndefLogFile == "")
			return(-1); // cant log to invalid file

		sensorsArr = new SensorConfig[numSensors+1]; // +1 for undefined log
		sensorsArr[i].logFile = UndefLogFile; // only filename is required
		i++;
	}
	else
	{
		sensorsArr = new SensorConfig[numSensors];
	}

	for (json::iterator it = configJson["Sensors"].begin(); it != configJson["Sensors"].end(); ++it)
	{
		// critical data (error if not defined)
		if(!(*it)["ID"].is_number())
		{
			deleteSensorsArray();
			return(-1);
		}
		sensorsArr[i].id = (*it)["ID"];

		if(!(*it)["Desc"].is_string())
		{
			deleteSensorsArray();
			return(-1);
		}
		sensorsArr[i].desc = (*it)["Desc"];

		if(!(*it)["Units"].is_string())
		{
			deleteSensorsArray();
			return(-1);
		}
		sensorsArr[i].unit = (*it)["Units"];

		// non critical data
		if((*it)["RelayRate"].is_number())
			sensorsArr[i].relayRate = (*it)["RelayRate"];
		else
			sensorsArr[i].relayRate = defaultRelayRate;

		if((*it)["RelayDest"].is_array())
		{
			sensorsArr[i].relayDestinationLength = (*it)["RelayDest"].size();
			sensorsArr[i].relayDestinationArr = new string[(*it)["RelayDest"].size()];
			int element = 0;
			for (json::iterator it2 = (*it)["RelayDest"].begin(); 
			     it2 != (*it)["RelayDest"].end(); ++it2)
			{
				if(!(*it2).is_string())
				{
					//must be string, if not error
					deleteSensorsArray();
					return(-1);
				}
				sensorsArr[i].relayDestinationArr[element] = (*it2);
				element++;
			}
		}
		else
		{
			// populate with default values if not defined
			sensorsArr[i].relayDestinationLength = defaultRelayDestLength;
			if(defaultRelayDestLength != 0)
			{
				sensorsArr[i].relayDestinationArr = new string[defaultRelayDestLength];
				for(int element = 0; element < defaultRelayDestLength; element++)
				{
					sensorsArr[i].relayDestinationArr[element] = defaultRelayDestArr[element];
				}
			}
			else
			{
				sensorsArr[i].relayDestinationArr = nullptr;
			}
		}

		if((*it)["LogFile"].is_string())
			sensorsArr[i].logFile = (*it)["LogFile"];
		else
			sensorsArr[i].logFile = defaultLogFile;

		sensorsArr[i].lastRelay = 0; // initialize last relay value
		i++;
	}

	// print settings if requested
	if(printSettings)
	{
		printLogSettings();
		printDefaultSettings();
	}

	// create and open required files
	if(createLogs() != 0)
	{
		return(-1);
	}

	// compleated witout errors
	return(0);
}

int sensorHandle::createLogs()
{

	string fileString = "";

	// create log directory
	if(logRoot != "")
	{
		fileString = logRoot;
		const int dir_err = mkdir(fileString.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(dir_err == -1 && errno != EEXIST)
		{
			cout << "Error creating directory: " << fileString << endl;
			return(-1);
		}
	}

	// create log subdirectory
	if(logSubFolder == "")
	{
		if(logRoot == "")
			fileString = "";
		else
			fileString = logRoot + "/";
	}
	else
	{
		fileString = logRoot + '/' + logSubFolder + '/';
		const int dir_err = mkdir(fileString.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
			cerr << "Error creating directory: " << fileString << endl;
			return(-1);
		}
	}

	int i = 0;
	//create default log file (only if default log is enabled All)
	if(logSetting == logStates::All)
	{
		ofstream* defaultLogPtr = new ofstream;
		defaultLogPtr->open(fileString + sensorsArr[i].logFile);
		if(!defaultLogPtr->is_open())
		{
			delete defaultLogPtr; // error opening file
			return(-1);
		}

		pair<string, ofstream*> tempPair (sensorsArr[i].logFile ,defaultLogPtr);
		fileMap.insert(tempPair); // commit to map

		// add header to file
		(*defaultLogPtr) << "ID\tData + units\tTime" << endl;

		numSensors++; // add dummy sensor for defLog
		i++; // change starting index
	}

	//create specified logs
	for(; i < numSensors; i++)
	{
		if(sensorsArr[i].logFile == "")
			continue; // no file associated with this sensor

		if(fileMap.find(sensorsArr[i].logFile) != fileMap.end())
		{
			// file already exists, continue to next
			//(*fileMap[sensorsArr[i].logFile]) << "Stuff";
			continue;
		}
		// doesn't exist, create new file
		ofstream* filePtr = new ofstream;
		filePtr->open(fileString + sensorsArr[i].logFile + ".txt");

		if(!filePtr->is_open())
		{
			// error opening file
			delete filePtr;
			// clean entire map
			 for ( unordered_map<string, ofstream*>::iterator it = fileMap.begin();
				it != fileMap.end();
				it++ )
			{
				it->second->close();
				delete it->second; // remove all identifications before removing map
			}
			return(-1);
		}
		// file open, wtite header and add to map

		(*filePtr) << "ID\tData + units\tTime" << endl;

		pair<string, ofstream*> tempPair (sensorsArr[i].logFile ,filePtr);
		fileMap.insert(tempPair); // commit to map
	}
	return(0);
}

int sensorHandle::writeToLog(int sensorID, string data)
{
	int arrIndex = -1;
	int i = 0;
	if(logSetting == logStates::All)
		i++; // first sensor is UndefLog

	// check if id is existant
	for(; i < numSensors; i++)
	{
		if(sensorsArr[i].id == sensorID)
		{
			arrIndex = i;
			break;
		}
	}

	if(arrIndex == -1)
	{
		// signal not defined, use the default log if set to do so
		if(logSetting == logStates::All)
		{
			(*fileMap.find(sensorsArr[0].logFile)->second) << data << endl;
			return(0);
		}
		return(-1);
	}

	// signal was defined, write to log
	(*fileMap.find(sensorsArr[arrIndex].logFile)->second) << data << endl;

	return(0);
}

int sensorHandle::relayPacket(int sensorID, std::string packet, udpSocketHandle* sendConn, bool overideRate)
{
	int arrIndex = -1;
	int i = 0;
	if(logSetting == logStates::All)
		i++; // first sensor is UndefLog

	// check if id is existant
	for(; i < numSensors; i++)
	{
		if(sensorsArr[i].id == sensorID)
		{
			arrIndex = i;
			break;
		}
	}

	if(arrIndex == -1)
	{
		// unkown ID, send to UnknownDest destination
		if(UnknownDestArr != nullptr)
		{
			for(int element = 0; element < UnknownDestLength ; element++)
			{
				int res = sendConn->sendDataTo(packet,UnknownDestArr[element]);
				if(res != 0)
					return(res); // error if send failed
			}
		}
		return(0);
	}

	if(sensorsArr[i].relayDestinationLength == 0)
		return(-1); // cannot send to no destination

	if(overideRate) // send packet regardless, if overriden
	{
		for(int element = 0; element < sensorsArr[i].relayDestinationLength ; element++)
		{
			int res = sendConn->sendDataTo(packet,sensorsArr[i].relayDestinationArr[element]);
			if(res != 0)
				return(res); // error if send failed
		}
		return(0); // no error
	}
	if(sensorsArr[i].relayRate < 1)
		return(0); // sensor is not set to relay

	if(sensorsArr[i].lastRelay == 0) // send if wait is 0
	{
		sensorsArr[i].lastRelay = sensorsArr[i].relayRate -1;

		for(int element = 0; element < sensorsArr[i].relayDestinationLength ; element++)
		{
			int res = sendConn->sendDataTo(packet,sensorsArr[i].relayDestinationArr[element]);
			if(res != 0)
				return(res); // error if send failed
		}
		return(0);
	}
	else
	{
		sensorsArr[i].lastRelay--;
	}

	return(0);
}

void sensorHandle::printLogSettings()
{
	cout << "Log Level: ";
	switch(logSetting)
	{
	case(logStates::All):
		cout << "All" << endl;
		break;
	case(logStates::DefOnly):
		cout << "DefOnly" << endl;
		break;
	case(logStates::FileOnly):
		cout << "FileOnly" << endl;
		break;
	}

	cout << "Log Root: " << logRoot << endl;
	cout << "Log Sub Folder: " << logSubFolder << endl;
}

void sensorHandle::printDefaultSettings()
{
	cout << "Default Relay Rate: " << defaultRelayRate << endl;
	cout << "Default Destination(s): ";
	for(int i = 0; i < defaultRelayDestLength; i++)
	{
		cout << "\"" << defaultRelayDestArr[i] << "\"";
		if(i + 1 != defaultRelayDestLength)
			cout << ", ";
	}
	cout << endl;

	cout << "Unknown Destination(s): ";
	for(int i = 0; i < UnknownDestLength; i++)
	{
		cout << "\"" << UnknownDestArr[i] << "\"";
		if(i + 1 != UnknownDestLength)
			cout << ", ";
	}
	cout << endl;

	cout << "Default Log File: " << defaultLogFile << endl;
}

void sensorHandle::deleteSensorsArray()
{
	if (sensorsArr != nullptr)
	{
		for(int i = 0; i < numSensors; i++)
		{
			if(sensorsArr[i].relayDestinationArr != nullptr)
				delete[] sensorsArr[i].relayDestinationArr;
		}
		delete[] sensorsArr;
	}
}

