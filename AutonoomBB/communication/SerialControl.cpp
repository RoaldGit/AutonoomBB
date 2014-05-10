/*
 * SerialControl.cpp
 *
 *  Created on: 8 mei 2014
 *      Author: Roald
 *
 *      Based on: http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html
 */

#include <termios.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "SerialControl.h"

#define SDEBUG true
#define _POSIX_SOURCE 1 /* POSIX compliant source */

using namespace std;

SerialControl::SerialControl()
	:fileDescriptor(0), serialThread(0)
{

}

SerialControl::~SerialControl()
{

}

void SerialControl::setup()
{
	struct termios newSettings;		// Contains config for the serial device
	fileDescriptor = open(SERIALDEVICE, O_RDWR | O_NOCTTY | O_NDELAY); // Open serial device, Read/Write.
							// No tty because linenoice can terminate the port

	// Ensure the device has been opened
	if(fileDescriptor < 0) cout << "Failed to open " << SERIALDEVICE << endl;
	assert(fileDescriptor >= 0);

	// Set all values in the struct to 0 (Memory block can contain random data)
	memset(&newSettings, 0, sizeof newSettings);

	// Change the config.
	// See http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html for a detailed description of each flag
	newSettings.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newSettings.c_iflag = IGNBRK | ICRNL;
	newSettings.c_oflag = 0;
	newSettings.c_lflag = ICANON;

	newSettings.c_cc[VINTR]    = 0;     /* Ctrl-c */
	newSettings.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	newSettings.c_cc[VERASE]   = 0;     /* del */
	newSettings.c_cc[VKILL]    = 0;     /* @ */
	newSettings.c_cc[VEOF]     = 4;     /* Ctrl-d */
	newSettings.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	newSettings.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	newSettings.c_cc[VSWTC]    = 0;     /* '\0' */
	newSettings.c_cc[VSTART]   = 0;     /* Ctrl-q */
	newSettings.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	newSettings.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	newSettings.c_cc[VEOL]     = 0;     /* '\0' */
	newSettings.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	newSettings.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	newSettings.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	newSettings.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	newSettings.c_cc[VEOL2]    = 0;     /* '\0' */

	// Clean the serial line and apply the new settings
	tcflush(fileDescriptor, TCIFLUSH);
	tcsetattr(fileDescriptor, TCSANOW, &newSettings);

	if(SDEBUG)
	{
		int debugFile = open("/dev/ttyO1", O_RDWR | O_NOCTTY | O_NDELAY);
		tcflush(debugFile, TCIFLUSH);
		tcsetattr(debugFile, TCSANOW, &newSettings);
	}

	usleep(1000);
	//char *message;
	unsigned char package[] = {0xFF, 0xFF, 0x07, 0xFE, 0x07, 0xFE, 0x00};
	//unsigned char package[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC0, 0x3E, 0x35, 0x01, 0x01};
	char incomingBuffer[UART_BUFFER_SIZE];
	memset(incomingBuffer, 0, UART_BUFFER_SIZE);

	cout << "Sending message" << endl;

	unsigned char lf = 0x0D;
	int bytes = write(fileDescriptor, &package, 7);
	write(fileDescriptor, &lf, 1);

	usleep(3000);

	int received = read(fileDescriptor, incomingBuffer, UART_BUFFER_SIZE);
	incomingBuffer[received] = 0;
	cout << incomingBuffer << endl;
	if(bytes == -1) cout << errno << endl;
	else cout << "Done sending " << bytes << " bytes" << endl;
}

