
#include <iostream>
#include <unistd.h>
#include <string>

#include "socket.h"
#include "dataProtocol.h"
#include "sensorHandle.h"

#include "jsonParse.h"
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;

int main()
{

	udpSocketHandle conn;
	conn.initializeRecive(2222);

	sensorHandle hSensor;
	if(hSensor.initFromConfig("Config.json", true) != 0)
	{
		return(0);
	}

	string output;

	/*	
		dataProtocol dataProt;
		string testident1;
		string testident2;

		dataIdentifier testDataIdentifier;
		testDataIdentifier.id = 1;
		testDataIdentifier.description = "Temperature sensor1";
		testDataIdentifier.units = "Celsius";
		dataProt.package(testDataIdentifier, &testident1);

		testDataIdentifier.id = 2;
		testDataIdentifier.description = "Humidity sensor1";
		testDataIdentifier.units = "%RH";
		dataProt.package(testDataIdentifier, &testident2);

		string testdata1;
		string testdata2;
		string testdata3;
		string testdata4;

		dataPacket testDataPacket;
		testDataPacket.id = 1;
		testDataPacket.data = "12.5";
		testDataPacket.time = "15.001";
		dataProt.package(testDataPacket, &testdata1);

		testDataPacket.id = 1;
		testDataPacket.data = "12.716";
		testDataPacket.time = "16.453";
		dataProt.package(testDataPacket, &testdata2);

		testDataPacket.id = 2;
		testDataPacket.data = "16.5";
		testDataPacket.time = "4.670";
		dataProt.package(testDataPacket, &testdata3);

		testDataPacket.id = 3;
		testDataPacket.data = "16.5";
		testDataPacket.time = "4.670";
		dataProt.package(testDataPacket, &testdata4);

		conn.sendDataTo(testident1, "localHost:2222");
		conn.sendDataTo(testident2, "localHost:2222");
		conn.sendDataTo(testdata1, "localHost:2222");
		conn.sendDataTo(testdata3, "localHost:2222");
		conn.sendDataTo(testdata2, "localHost:2222");
		//conn.sendDataTo(testdata4, "localHost:2222");
		sleep(1);
	*/

	cout << "\n---- Log Start ----" << endl;

	while(true)
	{
		int res = conn.popRecBufferStr(&output);
		if(res == -1)
			continue; // no data in buffer, wait

		hSensor.processPacket(output, &conn);

		//sleep(1);
	}

	conn.stopRecive();
	//dataLog.close();

	return(0);
}
