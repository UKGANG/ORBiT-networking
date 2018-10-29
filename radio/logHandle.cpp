#include "logHandle.h"

logHandle::logHandle()
{
//	cout << this << " " << &logFile << endl;
}

logHandle::~logHandle()
{
	if(logFile.is_open())
	{
		logFile.close();
	}
}

int logHandle::setLogFile(string logPath)
{
	if(logFile.is_open()) // avoid opening two logs
		return(-2);

	logFile.open(logPath, ofstream::out | ofstream::trunc);
	if(!logFile.is_open())
		return(-1);
	return(0);
}

int logHandle::writeToLog(string text)
{
	if(!logFile.is_open())
		return(-1);
	logFile << text;
}



