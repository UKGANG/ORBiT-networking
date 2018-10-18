#ifndef UDPSOCKET_HANDLE
#define UDPSOCKET_HANDLE

#include <string.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <netinet/in.h>
#include <unordered_map>

#define BUFFER_LENGTH 1024

struct linkedString
{
	int dataLength;
	char data[1024];
	linkedString* next;
	//linkedString(int length, char* dataPtr){next = nullptr; data = dataPtr; dataLength = length;};
	linkedString(){next = nullptr;};
	//~linkedString(){if(data != nullptr) {delete[] data;}};
};

class udpSocketHandle
{
public:
	udpSocketHandle();
	~udpSocketHandle();

	int initializeRecive(int port, int bufLength = 255,int concurrentCon = 255);
	void stopRecive();

	int sendDataTo(std::string data, std::string destination);

	int popRecBufferStr(std::string* data);

	int getRecBufLevel();

	void printSockaddr(struct sockaddr* input);

private:
	void initRecv();
	int reciveData(int socket);

	int getHostByName(std::string destination, struct sockaddr* result);

	int recBufferFill;
	linkedString* bufferHead;
	linkedString* bufferTail;
	int bufferLenght;
	int recPort;
	int concCon;

	std::unordered_map<std::string, sockaddr_storage*> knownHosts;

	std::atomic<int> ready;
	std::thread reciveThread;

};

#endif
