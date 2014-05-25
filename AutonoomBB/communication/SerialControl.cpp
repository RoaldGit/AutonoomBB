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
#include <iomanip> // for cout << hex << dec
#include <pthread.h>

#include "SerialControl.h"

using namespace std;

SerialControl::SerialControl()
	:fileDescriptor(0), serialThread(0), inUseMutex(PTHREAD_MUTEX_INITIALIZER)
{

}

SerialControl::~SerialControl()
{

}

void SerialControl::setup()
{
	struct termios tty;		// Contains config for the serial device
	fileDescriptor = open(SERIALDEVICE, O_RDWR | O_NOCTTY | O_NDELAY); // Open serial device, Read/Write.
							// No tty because linenoice can terminate the port

	// Ensure the device has been opened
	if(fileDescriptor < 0) cout << "Failed to open " << SERIALDEVICE << endl;
	assert(fileDescriptor >= 0);

	// Set all values in the struct to 0 (Memory block can contain random data)
	memset(&tty, 0, sizeof tty);
	tcgetattr(fileDescriptor, &tty);

	cfsetospeed(&tty, (speed_t)B115200);
	cfsetispeed(&tty, (speed_t)B115200);

	tty.c_cflag     &=  ~PARENB;        // Make 8n1
	tty.c_cflag     &=  ~CSTOPB;
	tty.c_cflag     &=  ~CSIZE;
	tty.c_cflag     |=  CS8;

	tty.c_cflag     &=  ~CRTSCTS;       // no flow control
	tty.c_cc[VMIN]      =   1;                  // read doesn't block
	tty.c_cc[VTIME]     =   5;                  // 0.5 seconds read timeout
	tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

	/* Make raw */
	cfmakeraw(&tty);

	/* Flush Port, then applies attributes */
	tcflush( fileDescriptor, TCIFLUSH );

//	// Change the config.
//	// See http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html for a detailed description of each flag
//	tty.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
//	tty.c_iflag = IGNBRK | ICRNL;
//	tty.c_oflag = 0;
//	tty.c_lflag = ICANON;
//
//	tty.c_cc[VINTR]    = 0;     /* Ctrl-c */
//	tty.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
//	tty.c_cc[VERASE]   = 0;     /* del */
//	tty.c_cc[VKILL]    = 0;     /* @ */
//	tty.c_cc[VEOF]     = 4;     /* Ctrl-d */
//	tty.c_cc[VTIME]    = 0;     /* inter-character timer unused */
//	tty.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
//	tty.c_cc[VSWTC]    = 0;     /* '\0' */
//	tty.c_cc[VSTART]   = 0;     /* Ctrl-q */
//	tty.c_cc[VSTOP]    = 0;     /* Ctrl-s */
//	tty.c_cc[VSUSP]    = 0;     /* Ctrl-z */
//	tty.c_cc[VEOL]     = 0;     /* '\0' */
//	tty.c_cc[VREPRINT] = 0;     /* Ctrl-r */
//	tty.c_cc[VDISCARD] = 0;     /* Ctrl-u */
//	tty.c_cc[VWERASE]  = 0;     /* Ctrl-w */
//	tty.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
//	tty.c_cc[VEOL2]    = 0;     /* '\0' */
//
//	// Clean the serial line and apply the new settings
//	tcflush(fileDescriptor, TCIFLUSH);
	tcsetattr(fileDescriptor, TCSANOW, &tty);

	usleep(1000);

	int pos = 800;

	unsigned char stat[] = {0xFF, 0xFF, 0x07, 0xFE, 0x07, 0xFE, 0x00};
	unsigned char reboot[] = {0xFF, 0xFF, 0x07, 0xFD, 0xF2, 0x08, 0xF6};
	unsigned char green[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC0, 0x3E, 0x35, 0x01, 0x01};
	unsigned char ledsOff[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC0, 0x3E, 0x35, 0x01, 0x00};
	unsigned char blue[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC2, 0x3C, 0x35, 0x01, 0x02};
	unsigned char red[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC4, 0x3A, 0x35, 0x01, 0x04};
	unsigned char statusError[] = {0xFF, 0xFF, 0x0B, 0xFD, 0x03, 0xC6, 0x38, 0x30, 0x02, 0x00, 0x00};
	unsigned char torque[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xA0, 0x5E, 0x34, 0x01, 0x60};
	unsigned char torqueOff[] = {0xFF, 0xFF, 0x0A, 0xFD, 0x03, 0xC0, 0x3E, 0x34, 0x01, 0x00};
//	unsigned char moveshit[] = {0xFF, 0xFF, 0x0C, 0xFD, 0x05, 0x88, 0x76, 40, 0x01, 0x0A, 0x0A, 0x3C};
	unsigned char move[] = {0xFF, 0xFF, 0x0C, 0xFD, 0x06, 0xFE, 0x00, 0x3C, (pos & 0x00FF), (pos & 0xFF00) >> 8, (0x08 & 0xFD), 0xFD};

	unsigned char incomingBuffer[UART_BUFFER_SIZE];
	memset(incomingBuffer, 0, UART_BUFFER_SIZE);

	send(statusError);
	sleep(1);
	send(torque);
//	usleep(3000);

	usleep(3000);
	send(green);

	sleep(1);
	send(blue);

	sleep(1);
	send(red);

	sleep(1);
	send(ledsOff);

	usleep(3000);

	sleep(1);
	//send(moveshit);
	send(move);
	sleep(2);
	send(stat);

	int received = read(fileDescriptor, incomingBuffer, UART_BUFFER_SIZE);
	incomingBuffer[received] = 0;
	cout << hex << incomingBuffer << dec << endl;
	// TODO make this a thread
}

void SerialControl::send(unsigned char buffer[])
{
	buffer[5] = calcCheck1(buffer);
	buffer[6] = calcCheck2(buffer[5]);

	int bytes = write(fileDescriptor, buffer, buffer[2]);
	if(bytes == -1) cout << errno << endl;

	cout << "Sent ";
	for(int i = 0; i < buffer[2]; i++)
		cout << hex << (int)buffer[i] << dec << " ";

	cout << endl;
}

unsigned char SerialControl::calcCheck1(unsigned char buffer[])
{
	int packSize =  buffer[2];
	unsigned char checksum1 = (buffer[2] ^ buffer[3] ^ buffer[4]);

	if(packSize > 7)
		for(int i = 7; i < packSize; i++)
			checksum1 ^= buffer[i];

	return checksum1 &= 0xFE;
}

unsigned char SerialControl::calcCheck2(unsigned char checksum1)
{
	return ~(checksum1) & 0xFE;
}
