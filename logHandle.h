#ifndef LOG_HANDLE
#define LOG_HANDLE

#include <fstream>
#include <string>
#include <iostream>

using namespace std;

// DO NOT DO THIS UNELSS YOU KNOW WHAT YOU'RE DOING!!!
// this implementation is probably wrong on many levels regardless
class logHandle
{
public:

	logHandle();
	~logHandle();

	int setLogFile(string logPath);
	int writeToLog(string text);

	template<class T>
	logHandle& operator<<(const T &in)
	{
		//cout << in << flush;
		logFile << in << flush;
		return(*this);
	}

	logHandle& operator<<(ostream& (*f)(ostream&))
	{
		//f(cout);
		f(logFile);
		return(*this);
	}

private:
	ofstream logFile;
};

extern logHandle hLog;


#endif
