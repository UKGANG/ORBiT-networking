# ORBiT Networking Libraries

## Radio

To handle data transmissions over the xBee and make sure that data is correct.

## Current Version
### Ver: 0.12

## Currently Implemented

- Serial handle
- xBee command handle (execution not implemented)
- Data checking
- Data reception acknowledge

## How to use

To transfer to the Beaglebone use following command and compile on the beaglebone

    scp -r bb_code root@192.168.7.2:/root

Notes:

- transfers entire directory!
- reconfigure target tty device before compile!

To run on remote system (i.e. as the Beaglebone) use following command:

    mkfifo /tmp/f ; cat /tmp/f | ./a.out -q | /bin/sh -i > /tmp/f 2>&1

Note: ./a.out is the compiled program. If you compiled the program under a different name, change ./a.out accordingly!

To compile the program, use the command:

    g++ -lpthread *.cpp

## TODO
### Add/Implement

- Issue xBee commands during runtime
- Proper handshaking

### Potential Features

- Packet priorities
- Packet addresses
- stream opperators

### Refine

- Data validation (see "Proper handshaking")
- Add more xBee commands
- Some functions missing proper return values
- Command line arguments
- quiet mode
- input handle (backspace)

## Relay

To relay and log data comming from signals or programs to other programs.


## Credits

Cem Eden
