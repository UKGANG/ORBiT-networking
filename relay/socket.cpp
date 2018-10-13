
#include "socket.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>

using namespace std;

udpSocketHandle::udpSocketHandle()
{
	recBufferFill = 0;
	bufferHead = nullptr;
	ready = 0; // initialize ready state for threads
}

udpSocketHandle::~udpSocketHandle()
{

	if (reciveThread.joinable())
	{
		reciveThread.join();
	}

	linkedString* curPtr = bufferHead;
	if(curPtr == nullptr)
		return;
	while(curPtr->next != nullptr)
	{
		linkedString* tempPtr = curPtr;
		curPtr = curPtr->next;
		delete tempPtr;
	}
	delete curPtr;
	
}

void udpSocketHandle::stopRecive()
{
	cout << "Stopping thread..." << endl;
	ready = 0;
	if (reciveThread.joinable())
	{
		reciveThread.join();
	}
}

void udpSocketHandle::initRecv()
{
	struct sockaddr_in serv_addr, cli_addr;

	int sock = socket(AF_INET, SOCK_DGRAM, 0); //open socket SOCK_STREAM = TCP SOCK_DGRAM = UDP
	if (sock == -1)
	{
		cerr << "Error opening socket!" << endl;
		ready = -1;
		return;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(recPort);

	if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) //bind socket
	{
		cerr << "Error binding!" << endl;
		ready = -1;
		return;
	}

	ready = 1;
	while(ready == 1)
	{
		reciveData(sock);
		//cout << "data recived" << endl;
	}
}

int udpSocketHandle::reciveData(int socket)
{

	//bufferHead = new linkedString(bufferLenght);

	//int tempLength = recv(socket, nullptr, 0, MSG_PEEK); //allocate buffer for message
	//linkedString* tempLStringPtr = new linkedString(tempLength, new char[tempLength]);
	linkedString* tempLStringPtr = new linkedString();

	tempLStringPtr->dataLength = recv(socket, &(tempLStringPtr->data), BUFFER_LENGTH, 0); // read message into buffer

	if(bufferHead == nullptr)
	{
		bufferHead = tempLStringPtr;
		bufferTail = tempLStringPtr;
	}
	else
	{

		bufferTail->next = tempLStringPtr;
		bufferTail = tempLStringPtr;
	}
	recBufferFill++;
	//cout << "Buffer Fill: " << recBufferFill << " Data: " << tempLStringPtr->data <<endl;
	return(0);
}

int udpSocketHandle::initializeRecive(int port, int bufLength,int concurrentCon)
{
	recPort = port;
	bufferLenght = bufLength;
	concCon = concurrentCon;
	
	reciveThread = thread(&udpSocketHandle::initRecv, this);

	//cout << "thread started" << endl;
	while(ready == 0){ } // wait for initialization to complete

	if(ready == 1)
	{
		return(0);
	}
	else
	{
		reciveThread.join();
		return(-1);
	}

	return(0);
}

int udpSocketHandle::popRecBufferStr(string* data)
{
	if(recBufferFill == 0)
		return(-1);

	*data = ""; // clear string before writing

	int bufLength = bufferHead->dataLength;
	for(int i = 0; i < bufLength; i++)
	{
		data->push_back(bufferHead->data[i]);
		//cout << bufferHead->data[i] << flush;
	}
	recBufferFill--;
	linkedString* tempLStringPtr = bufferHead;
	if(bufferHead->next == nullptr)
	{
		bufferHead = nullptr;
		bufferTail = nullptr;
	}
	else
	{
		bufferHead = bufferHead->next;
	}
	delete tempLStringPtr;
	return(bufLength);
}

int udpSocketHandle::getRecBufLevel()
{
	return(recBufferFill);
}

int udpSocketHandle::sendDataTo(string data, string destination)
{
	struct sockaddr_in serv_addr, cli_addr;
	struct hostent *target;

	int colonLocation = destination.find_first_of(":");
	if(colonLocation == string::npos)
	{
		cerr << "The address: \"" << destination << "\" Is not valid!" << endl;
		return(-1);
	}

	string tempAddr = destination.substr(0, colonLocation);
	string tempPort = destination.substr(colonLocation + 1, destination.length() - (colonLocation + 1));

	int tempPortInt = strtol( tempPort.c_str(), nullptr, 10);
	if(tempPortInt == 0)
	{
		cerr << "The port: \"" << tempPort << "\" Is not valid!" << endl;
		return(-1);
	}

	target = gethostbyname(tempAddr.c_str());
    	if (target == NULL) {
		cerr << "Error, no such host!" << endl;
		return(-1);
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0); //open socket SOCK_STREAM = TCP SOCK_DGRAM = UDP
	if (sock == -1)
	{
		cerr << "Error opening socket!" << endl;
		return(-1);
	}
	
	// set up sender addresses (namely make the system do it for us)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	//bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(0);

	if(bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) //bind socket
	{
		cerr << "Error binding!" << endl;
		return(-1);
	}

	// set up reciver address using input arguments
	cli_addr.sin_family = AF_INET;
	bcopy((char *)target->h_addr, (char *)&cli_addr.sin_addr.s_addr, target->h_length);
	cli_addr.sin_port = htons(tempPortInt);

	sendto(sock, data.c_str(), data.length(), 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));

	return(0);
}

