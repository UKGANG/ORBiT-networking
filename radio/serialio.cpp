
#include "serialio.h"

serialIO::~serialIO()
{
	if(fd != -1)
		::close(fd);
}

int serialIO::initialize(string device, speed_t speed, int parity, bool blocking, int timeoutMs)
{
	fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd == -1)
	{
		cout << "failed to open port" << endl;
		return(-1);
	}

	if(setAttributes(speed, parity) != 0)
		return(-1);
	if(setBlocking(blocking, timeoutMs) != 0)
		return(-1);

	ioctl(fd, TCSAFLUSH);

	settings.device = device;
	settings.speed = speed;
	settings.parity = parity;
	settings.blocking = blocking;
	settings.timeoutMs = timeoutMs;

	return(0);
}

int serialIO::initialize(serialConfig config)
{
	return(initialize(config.device, config.speed, config.parity, config.blocking, config.timeoutMs));
}

int serialIO::close()
{
	return(::close(fd));
}

int serialIO::setAttributes(speed_t speed, int parity)
{
        struct termios tty;
        memset(&tty, 0, sizeof(tty));
        if (tcgetattr (fd, &tty) != 0)
        {
                cout << "error at tcgetattr" << endl;
                return(-1);
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout


        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl


        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                cout  << "error at tcsetattr" << endl;
                return(-1);
        }
        return 0;
}

int serialIO::setBlocking(bool state, int timeoutMs)
{
	struct termios tty;
        memset(&tty, 0, sizeof(tty));
        if (tcgetattr (fd, &tty) != 0)
        {
                cout << "error at tggetattr" << endl;
                return(-1);
        }

        tty.c_cc[VMIN]  = state;
        tty.c_cc[VTIME] = timeoutMs;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
                cout <<  "error setting term attributes" << endl;
		return(-1);
	}
	return(0);
}

int serialIO::writeTo(string toWrite)
{

	char* tempString = new char [toWrite.length()];

	for(int i = 0; i < toWrite.length(); i++)
	{
		tempString[i] = toWrite[i];
	}

	return(write(fd, tempString,toWrite.length()));
}

string serialIO::readFrom()
{
	char buf [256];
	int length = read(fd, buf, sizeof(buf));
	if(length > 0)
	{
		string temp = "";
		temp.append(buf, length);
		return(temp);
	}
	return(string(""));
}

string serialIO::readFrom(int minChars)
{
	timespec ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

	double sTime = ts.tv_nsec;
	char buf [256];
	int totRecived = 0;
	string tempString = "";
	while(totRecived < minChars)
	{
		int tempNum = read(fd, buf, sizeof(buf));
		if(tempNum > 0)
		{
			totRecived += tempNum;
			tempString.append(buf, tempNum);

			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
			sTime = ts.tv_nsec;
		}

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
		if( ts.tv_nsec - sTime < timeoutTime)
			return(tempString);
	}

	return(tempString);
}

void serialIO::setRTS()
{
	int state;
	ioctl(fd, TIOCMGET, &state);
	state |= TIOCM_RTS;
	ioctl(fd, TIOCMSET, &state);
}

void serialIO::clearRTS()
{
	int state;
	ioctl(fd, TIOCMGET, &state);
	state &= ~TIOCM_RTS;
	ioctl(fd, TIOCMSET, &state);
}

void serialIO::setDTR()
{
	int state;
	ioctl(fd, TIOCMGET, &state);
	state |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &state);
}

void serialIO::clearDTR()
{
	int state;
	ioctl(fd, TIOCMGET, &state);
	state &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &state);
}

void serialIO::getConfig(serialConfig *config)
{
	config->device = settings.device;
	config->speed = settings.speed;
	config->parity = settings.parity;
	config->blocking = settings.blocking;
	config->timeoutMs = settings.timeoutMs;
}
