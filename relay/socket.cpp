#include "socket.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <cstring>

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

	for(unordered_map<std::string, sockaddr_storage*>::iterator it = knownHosts.begin();
		it != knownHosts.end();
		it++)
	{
		delete it->second; // remove all identifications before removing map
	}
}

void udpSocketHandle::stopRecive()
{
	ready = 0;
	if (reciveThread.joinable())
	{
		reciveThread.join();
	}
}

void udpSocketHandle::initRecv()
{
	struct sockaddr_in6 serv_addr, cli_addr;

	int sock = socket(AF_INET6, SOCK_DGRAM, 0); //open socket SOCK_STREAM = TCP SOCK_DGRAM = UDP
	if (sock == -1)
	{
		cerr << "Error opening socket!" << endl;
		ready = -1;
		return;
	}

	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_addr = in6addr_any;
	serv_addr.sin6_port = htons(recPort);

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

	int res = 0;
	do
	{
		res = recv(socket, &(tempLStringPtr->data), BUFFER_LENGTH, MSG_DONTWAIT); // read message into buffer
		if(res == -1) // check if an error occured
		{
			if (errno != EAGAIN || errno != EWOULDBLOCK) //error was unexpected
			{
				delete tempLStringPtr;
				return (res);
			}
			else if(ready != 1) // error expected, but thread set to terminate
			{
				delete tempLStringPtr;
				return(0);
			}
		}
	} while(res == -1);

	tempLStringPtr->dataLength = res;

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

void udpSocketHandle::printSockaddr(struct sockaddr* input)
{
	cout << "\n***************\nSockaddr:\n" << "sa_family: " << input->sa_family << "\nsa_data:";
	for(int i = 0; i < 14; i++)
	{
		cout << (int)input->sa_data[i] << " ";
	}
	cout << "\n";

	// not the most elegant casting, but only used for debuging
	sockaddr_in* temp = (struct sockaddr_in*)input;
	sockaddr_in6* temp6 = (struct sockaddr_in6*)input;

	switch(input->sa_family)
	{
	case(AF_INET):
		cout << "********\nIPv4:\n";

		cout << "sin_family: " << (int)temp->sin_family << "\n"
		     << "sin_port: " << ntohs(temp->sin_port) << "\n"
		     << "sin_addr: ";

		for(int i = 0; i < 4; i++)
		{
			cout << (int)((char*)&temp->sin_addr.s_addr)[i];
			if(i+1 < 4)
				cout << ".";
			else
				cout << "\n";
		}
		break;

	case(AF_INET6):
		cout << "********\nIPv6:\n";

		cout << "sin6_family: " << (int)temp6->sin6_family << "\n"
		     << "sin6_port: " << ntohs(temp6->sin6_port) << "\n"
		     << "sin6_flowinfo: " << (int)temp6->sin6_flowinfo << "\n"
		     << "sin6_addr: ";
		for(int i = 0; i < 16; i += 2)
		{
			cout << hex << (int)temp6->sin6_addr.s6_addr[i] << (int)temp6->sin6_addr.s6_addr[i+1];
			if(i+2 != 16)
				cout << ":";
			else
				cout << "\n" << dec;
		}
		cout << "sin6_scope_id: " << (int)temp6->sin6_scope_id << "\n";
		break;
	}

	cout << "***************\n" << endl;
}

int udpSocketHandle::sendDataTo(string data, string destination)
{

	//struct sockaddr cli_addr, serv_addr;
	struct sockaddr_storage serv_data;
	struct sockaddr* cli_addr;// = ((struct sockaddr*)&cli_data);
	struct sockaddr* serv_addr = ((struct sockaddr*)&serv_data);

	//struct sockaddr_storage* pCliData;

	bool newHost;

	if(knownHosts.count(destination))
	{
		// host already exists
		cli_addr = (struct sockaddr*)knownHosts[destination];
		newHost = false;
	}
	else
	{
		//doesnt exist, query new
		cli_addr = (struct sockaddr*)(new struct sockaddr_storage);
		if(getHostByName(destination, cli_addr))
		{
			delete (struct sockaddr_storage*)cli_addr;
			return(-1);
		}
		newHost = true;
	}

//	printSockaddr(cli_addr);

	switch(cli_addr->sa_family)
	{
	case(AF_INET):
		((struct sockaddr_in*)serv_addr)->sin_family = AF_INET;
		((struct sockaddr_in*)serv_addr)->sin_addr.s_addr = INADDR_ANY;
		((struct sockaddr_in*)serv_addr)->sin_port = htons(0);
		break;

	case(AF_INET6):
		((struct sockaddr_in6*)serv_addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6*)serv_addr)->sin6_addr = in6addr_any;
		((struct sockaddr_in6*)serv_addr)->sin6_port = htons(0);
		break;
	}


	int sock = socket(cli_addr->sa_family, SOCK_DGRAM, 0); //open socket SOCK_STREAM = TCP SOCK_DGRAM = UDP
	if (sock == -1)
	{
		if(newHost)
			delete (struct sockaddr_storage*)cli_addr;
		cerr << "Error opening socket!" << endl;
		return(-1);
	}

	switch(cli_addr->sa_family)
	{
	case(AF_INET):
		// set up sender addresses (namely make the system do it for us)
		if(bind(sock, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in)) == -1) //bind socket
		{
			if(newHost)
				delete (struct sockaddr_storage*)cli_addr;
			cerr << "Error binding! (" << strerror(errno) << ")" << endl;
			return(-1);
		}
		sendto(sock, data.c_str(), data.length(), 0, (struct sockaddr*) cli_addr, sizeof(sockaddr_in));
		close(sock);
		break;
	case(AF_INET6):
		// set up sender addresses (namely make the system do it for us)
		if(bind(sock, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in6)) == -1) //bind socket
		{
			if(newHost)
				delete (struct sockaddr_storage*)cli_addr;
			cerr << "Error binding! (" << strerror(errno) << ")" << endl;
			return(-1);
		}
		sendto(sock, data.c_str(), data.length(), 0, (struct sockaddr*) cli_addr, sizeof(sockaddr_in6));
		close(sock);
		break;
	}

	// host was valid, add to map
	if(newHost)
	{
		knownHosts[destination] = (struct sockaddr_storage*)cli_addr;
	}
	return(0);
}

int udpSocketHandle::getHostByName(string destination,struct sockaddr* result)
{
	struct addrinfo* target;

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

	int res = getaddrinfo(tempAddr.c_str(), tempPort.c_str(), NULL, &target);
	if (res != 0)
	{
		cerr << "Error, no such host! (" << res << ")" << endl;
		return(-1);
	}

	// set up reciver address using input arguments
	int sock;
	addrinfo* tempTarget;

	switch(target->ai_family)
	{
	case(AF_INET):
		memcpy(result, target->ai_addr, sizeof(sockaddr_in));
		break;

	case(AF_INET6):
		memcpy(result, target->ai_addr, sizeof(sockaddr_in6));
		break;
	default:
		cerr << "Unkown Family";
		return(-1);
		break;
	}

	freeaddrinfo(target);
	return(0);
}
