#include "CppSocket.h"
#include <time.h>
#define DEFAULT_TIME_OUT 10000
#define byte char
#define DEFAULT_BUFFER_SIZE 4096

CppSocket::CppSocket(Service _service,int _socketfd,InterAddr _addr){
    //save infomation to object
    timeout = DEFAULT_TIME_OUT;
    localAddress = _addr;
    socketfd = _socketfd;
    service = _service;
    //set socket to non-blocking
    int flags = fcntl(socketfd, F_GETFL, 0);
    //when the socket is setted to TCP client mode, do non-blocking after connected to server
    if (_service != TCP_Client)
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
    // set up socket base on the service type
    SetUp();
    // init all of the indicator
    isSetValue=true;
    isConnectedValue=false;
    isClosedValue=false;
}
CppSocket::CppSocket(Service _service,InterAddr _addr){
    //save infomation to object
    timeout = DEFAULT_TIME_OUT;
    service = _service;
    localAddress =_addr;
    //create file describor base on service type
    if(service == UDP)
        socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    else
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
    //set socket to non-blocking
    int flags = fcntl(socketfd, F_GETFL, 0);
    //when the socket is setted to TCP client mode, do non-blocking after connected to server
    if (_service != TCP_Client)
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
    // set up socket base on the service type
    SetUp();
    // init all of the indicator
    isSetValue = true;
    isConnectedValue = false;
    isClosedValue = false;
}
CppSocket::CppSocket(Service _service,char* addr,int port){
    //save infomation to object
    timeout=DEFAULT_TIME_OUT;
    service=_service;
    //create file describor base on service type
    if(service==UDP)
        socketfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    else
        socketfd=socket(AF_INET, SOCK_STREAM, 0);
     //set socket to non-blocking
    int flags = fcntl(socketfd, F_GETFL, 0);
    //when the socket is setted to TCP client mode, do non-blocking after connected to server
    if(_service!=TCP_Client)
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
    //setting up local ip address
    localAddress.sin_family=AF_INET;
	localAddress.sin_addr.s_addr=inet_addr(addr);
	localAddress.sin_port=htons(port);
	// set up socket base on the service type
	SetUp();
	// init all of the indicator
    isConnectedValue=false;
    isClosedValue=false;
    isSetValue=true;
}

Error CppSocket::sendData(TransData* data){
    //send data base on service type
    if(service==UDP)
        return sendUDPData(data);
    else if(service==TCP_Client)
        return sendTCPClientData(data);
    else if(service==TCP_Server)
        return sendTCPServerData(data);
}
void CppSocket::sendData(TransData* data,ErrorHandler handler){
    //execute operation base receive error
    Error e=sendData(data);
    //execute error processor
    handler(e);
}

Error CppSocket::recevieData(int length,TransData* data){
    //recevie data base on data type
    if(service==UDP)
        return recevieUDPData(length,data);
    else if(service==TCP_Client)
        return recevieTCPClientData(length,data);
    else if(service==TCP_Server)
        return recevieTCPServerData(length,data);
}
void CppSocket::recevieData(int length,TransData* data,ErrorHandler handler){
    //execute operation base receive error
    Error e=recevieData(length,data);
    //execute error processor
    handler(e);
}

Error CppSocket::connectTo(InterAddr remoteAddr){
    // this function can only use for TCP Client
    if(service==TCP_Server&&service==UDP)
        return INCURRECT_SERVICE;
    // set socket connect to server
    int res = connect(socketfd, (struct sockaddr *)&remoteAddr, sizeof(struct sockaddr_in));
    //socket connected
    if (0 == res){
        printf("socket connect succeed immediately.\n");
     }else{
        //connector in progress
        if (errno == EINPROGRESS){
            fd_set rfds, wfds;
            struct timeval tv;
            // set fire describor list
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(socketfd, &rfds);
            FD_SET(socketfd, &wfds);tv.tv_sec = timeout;tv.tv_usec = 0;
            //wait for event
            int selres = select(socketfd + 1, &rfds, &wfds, NULL, &tv);
            //find out the response
            switch (selres){
            case -1:
                return SOCKET_ERROR;
            case 0:
                return TIMEOUT;
            default:
                // try recnnect
                if (FD_ISSET(socketfd, &rfds) || FD_ISSET(socketfd, &wfds)){
                    connect(socketfd, (struct sockaddr *)&remoteAddr, sizeof(struct sockaddr_in));
                    int err = errno;
                    // when connection broken set to not connected
                    if(err == EISCONN){
                        return CONNECTING_ERROR;
                    }
                    // print out error
                    else{
                        printf("connect failed. errno = %d\n", errno);
                        printf("FD_ISSET(sock_fd, &rfds): %d\n FD_ISSET(sock_fd, &wfds): %d\n", FD_ISSET(socketfd, &rfds) , FD_ISSET(socketfd, &wfds));
                        return CONNECTING_ERROR;
                    }
                }
            }
        }else{
            return CONNECTING_ERROR;
        }
     }
     // set up indicator
     isConnectedValue=true;
     // load remote access
     remoteAddress=remoteAddr;
     // set up the socket to non-blocking
     int flags = fcntl(socketfd, F_GETFL, 0);
     fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
    return NOERROR;
}
void CppSocket::connectTo(InterAddr remoteAddr,ErrorHandler errorHandler,VerfyHandler verfyHandler){// this function can only use for TCP Client
     // this function do not work for TCPSERVER and UDP
     if(service==TCP_Server&&service==UDP){
        //execute error processor
        errorHandler(INCURRECT_SERVICE);
        return;
     }
     Error e=connectTo(remoteAddr);
     // verfy this connection
     verfyHandler(this);
     //execute error processor
     errorHandler(e);
}

CppSocket* CppSocket::acceptClient(ErrorHandler errorHandler,VerfyHandler verfyHandler){
    int client_sockfd;
    InterAddr remote_addr;
    unsigned int sin_size=sizeof(struct sockaddr_in);
    //accept connection request and return a fire describor and address
    if((client_sockfd=accept(socketfd,(struct sockaddr *)&remote_addr,&sin_size))<0){
        Error e;
        if(errno==EAGAIN||errno==EWOULDBLOCK){
            e=NON_CLIENT_IN_LINE;//no connection request
        }else if(errno=ECONNABORTED){
            e=END_CONNECTION;// fd is closed
        }else if(errno=EBADF){
            e=NOT_FD;// please put a real fd
        }
        errorHandler(e);//execute error processor
        return NULL;
	}
    CppSocket* socket=new CppSocket(TCP_Client,client_sockfd,localAddress);// create a cppsocket to handle the connection
    socket->remoteAddress=remote_addr;//setting up remote address
    if(verfyHandler(socket)){
        socket->isConnectedValue=true;
        return socket;//return socket if it is verified
        //TODO: free socket when it not passed
    }
    return NULL;
}

Error CppSocket::stopSocket(){
    isClosedValue=true;
}
Error CppSocket::reconnect(){
    // this function can only use for TCP Client
}
Error CppSocket::setTimeOut(int time){
    // time in millier second
    timeout=time;
    return NOERROR;
}

bool CppSocket::isSocketSet(){
    return isSetValue;
}
bool CppSocket::isConnected(){
    return isConnectedValue;
}
bool CppSocket::isClosed(){
    return isClosedValue;
}


Error CppSocket::TCPClientSetUp(){
    return NOERROR;
}
Error CppSocket::TCPServerSetUp(){
    //bind socket to a local address
    if(bind(socketfd,(struct sockaddr *)&localAddress,sizeof(struct sockaddr))<0)
		return BINDING_ERROR;
    // set up listener length
    if(listen(socketfd,5)<0)
        return LISTENING_ERROR;
    return NOERROR;
}
Error CppSocket::UDPSetUp(){
    //bind socket to a local address
    if (bind(socketfd, (sockaddr *)&localAddress, sizeof(localAddress)) == -1) {
        return BINDING_ERROR;
    }
    return NOERROR;
}
Error CppSocket::SetUp(){
    //set up socket base on data type
    if(service==UDP)
        UDPSetUp();
    else if(service==TCP_Client)
        TCPClientSetUp();
    else if(service==TCP_Server)
        TCPServerSetUp();
}

Error CppSocket::sendTCPClientData(TransData* data){
    // read data from from TransData
    char* sendingData=data->dataBuff;
    int sendingLength=data->length;
    InterAddr addr=data->address;
    //mark the beginning time
    clock_t init=clock();
    int readLength=0;
    while(1){
        // send data to socket
         int res=send(socketfd,getRemain(readLength,sendingLength,sendingData),sendingLength,0);

         if(res==sendingLength)
            return NOERROR;
         if(res<0){
            if(errno==EAGAIN){//if socket is busy wait for later
                if(clock()-init>timeout)
                    return TIMEOUT;
            }else{
                return SOCKET_ERROR;
            }
         }else if(res==(sendingLength-readLength)){
            return NOERROR;
         }else{
            readLength=res;//mark the length is sended
         }
    }
}
Error CppSocket::sendTCPServerData(TransData* data){
    // TCPServer do not support sending and receiving data
    return INCURRECT_SERVICE;
}
Error CppSocket::sendUDPData(TransData* data){
     // read data from from TransData
    char* sendingData=data->dataBuff;
    int sendingLength=data->length;
    InterAddr addr=data->address;
     //mark the beginning time
    if(sendingLength>DEFAULT_BUFFER_SIZE)
        return INVALID_SIZE;
    //mark the beginning time
    clock_t init=clock();
    int readLength=0;
    while(1){
        // send packet to socketfd
         int res=sendto(socketfd,sendingData,sendingLength,0,(struct sockaddr*)&addr,sizeof(addr));
         if(res==sendingLength)
            return NOERROR;
         if(res<0){
            if(errno==EAGAIN){//if socket is busy wait for later
                if(clock()-init>timeout)
                return TIMEOUT;
            }else{
                return SOCKET_ERROR;
            }
         }else if(res==(sendingLength-readLength)){
            return NOERROR;
         }else{
            readLength=res;//mark the length is sended
         }
    }
}

Error CppSocket::recevieTCPClientData(int length,TransData* data){
    Error e=NOERROR;
    // set up receiving buffer
    char* result=(char*)malloc(sizeof(char)*length);
    char* buff=(char*)malloc(sizeof(char)*length);
    int unreadLength=length;
    //mark the beginning time
    clock_t init=clock();
    while(unreadLength>0){
        // receive packet
        int len=recv(socketfd,buff,unreadLength,0);
        // if socket closed, throw error
        if(isClosedValue){
            e=SOCKET_CLOSE;
            break;
        }
        // close the connection on the opposite side
        if(len<0&&errno==ECONNRESET){
            e=CONNECTING_ERROR;
            break;
        }
        else if(len<0){
            if(errno==EAGAIN){// when socket is not ready wait for next loop
                if(clock()-init>timeout)
                return TIMEOUT;// when time out throw exception
            }else{
                return SOCKET_ERROR;
            }
         }else if(len<=unreadLength){
            unreadLength=unreadLength-len;
            memcpy(result,buff,len);//copy data to result byte array
        }
    }
    free(buff);// free buffer
    // load data to TransData pointer
    data->address=remoteAddress;
    data->dataBuff=result;
    data->length=length-unreadLength;
    return e;
}
Error CppSocket::recevieTCPServerData(int length,TransData* data){
    // TCPServer do not support sending and receiving data
    return INCURRECT_SERVICE;
}
Error CppSocket::recevieUDPData(int length,TransData* data){
    // set up receiving buffer and remoteAddress
    byte* buff=(byte*)malloc(sizeof(char)*DEFAULT_BUFFER_SIZE*2);
    InterAddr remoteAddress;
    unsigned int len=sizeof(remoteAddress);
    int n=0;
    //mark the beginning time
    clock_t init=clock();
    while(n<1){
        //receive packet from socket
        n = recvfrom(socketfd, buff, DEFAULT_BUFFER_SIZE, 0, (sockaddr *)&remoteAddress, &len);
        if(n==-1&&(errno==EAGAIN||errno==EWOULDBLOCK)) {
            if(clock()-init>timeout)
                return TIMEOUT;
        }else if(n==-1){
            return SOCKET_ERROR;// return if socket have error
        }
    }
    char* result=(char*)malloc(sizeof(char)*n);
    memcpy(result,buff,n);
    // save data to TransData
    data->address=remoteAddress;
    data->dataBuff=result;
    data->length=n;
    free(buff);
    return NOERROR;
}
char* CppSocket::getRemain(int finishLength,int total,char* sdata){
    char* ddata=(char*)malloc((total-finishLength)*sizeof(char));
    memcpy(ddata,sdata+finishLength,(total-finishLength)*sizeof(char));
    return ddata;
}
CppSocket::~CppSocket(){

}

