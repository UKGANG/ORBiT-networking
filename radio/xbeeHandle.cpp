
#include "xbeeHandle.h"

xbeeHandle::xbeeHandle(serialIO* serial)
{
	serialHandle = serial;
}


string xbeeHandle::binaryCommand(string cmdStr, int reciveLength)
{
	serialHandle->clearRTS();
	serialHandle->writeTo(cmdStr);
	string returnString = "";
	if(reciveLength > 0)
		returnString = serialHandle->readFrom(reciveLength);
	serialHandle->setRTS();
	return(returnString);
}

int xbeeHandle::tryReset()
{
	//ATRT1
	usleep(2000);
	serialHandle->writeTo("+++");
	usleep(1500);
	serialHandle->writeTo("ATRT 1\n");

	string returnString = "";
	returnString = serialHandle->readFrom(1);

	cout << returnString << endl;

	if(returnString == "OK\r")
		return(0);
	return(-1);
}

float xbeeHandle::getBoardVoltage()
{
	string s = binaryCommand("\x3B", 6);

	if(s.length() == 6)
	{
		s.resize(5);  // remove extra cartrage return
		char * p;
		long n = strtoul( s.c_str(), & p, 16 );
		if ( * p == 0 )
			return(n / 65536.0f);
	}
	return(-1);
}

int xbeeHandle::getReceivedSignalStrength()
{
	string s = binaryCommand("\xb6", 2);

	if(s.length() == 2)
	{
		return(-2);
	}
	else if(s.length() == 1)
	{
		return((int)s.at(0));
	}
	return(-1);

	//cout << s << " " << s.length() << endl;

}

int xbeeHandle::setDestinationAddress(unsigned int address)
{
	if(address > 0xffff)
		return (-1);

	string s(1,'\0');
	s.append(1,address & 0xff);
	s.append(1, address >> 8);
	binaryCommand(s, 0);

	return (0);
}

int xbeeHandle::getDestinationAddress()
{
	string s = binaryCommand("\x80", 1);

	if(s.length() == 2)
		return((int)s.at(1) * 0xff + (int)s.at(0));
	return(-1);
}

int xbeeHandle::setSourceAddress(unsigned int address)
{
	if(address > 0xffff)
		return (-1);

	string s(1,'\x2A');
	s.append(1,address & 0xff);
	s.append(1, address >> 8);
	binaryCommand(s, 0);
	return(0);
}

int xbeeHandle::getSourceAddress()
{
	string s = binaryCommand("\xAA", 1);

	if(s.length() == 2)
		return((int)s.at(1) * 0xff + (int)s.at(0));
	return(-1);
}

int xbeeHandle::setInterfaceDataRate(int rate)
{

	int rateParameter;
	switch(rate)
	{
	case(B1200):
		rateParameter = 0;
		break;
	case(B2400):
		rateParameter = 1;
		break;
	case(B4800):
		rateParameter = 2;
		break;
	case(B9600):
		rateParameter = 3;
		break;
	case(B19200):
		rateParameter = 4;
		break;
	case(B38400):
		rateParameter = 5;
		break;
	case(B57600):
		rateParameter = 6;
		break;
	default:
		return(-1); // baud rate dosn't exist for the xbee
	}

	string s(1, 0x15); // change register
	s.append(1, rateParameter);
	s.append(1, 0x00);
	s.append(1, 0x09); // commit change
	s.append(2, 0x00);

	binaryCommand(s, 0); // sedn command & commit

	serialIO::serialConfig serConf; //get current settings
	serialHandle->getConfig(&serConf);
	serialHandle->close(); // close previous connection
	serConf.speed = rate;
	int res = serialHandle->initialize(serConf); // reopen connection with new baud rate
	if(res != 0)
		return(res);
	sleep(1);
	return(0);
}
int xbeeHandle::getInterfaceDataRate()
{
	string s = binaryCommand("\x95",4);
	if(s.length() == 4)
		return(s[3]*0xffffff + s[2]*0xffff + s[1]*0xff + s[0]);
	return(-1);
}


