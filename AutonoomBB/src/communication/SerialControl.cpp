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

SerialControl* SerialControl::uniqueInstance = NULL;

SerialControl::SerialControl()
	:fileDescriptor(0), serialThread(0), inUseMutex(PTHREAD_MUTEX_INITIALIZER)
{

}

SerialControl::~SerialControl()
{

}

SerialControl* SerialControl::getInstance()
{
	if(!uniqueInstance)
		uniqueInstance = new SerialControl;
	return uniqueInstance;
}

void SerialControl::setup()
{
	struct termios tty;		// Contains config for the serial device

	// Open serial device, Read/Write.
	// No controlling tty because linenoice can terminate the port
	fileDescriptor = open(SERIALDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);

	// Ensure the device has been opened
	if(fileDescriptor < 0) cout << "Failed to open " << SERIALDEVICE << endl;
	assert(fileDescriptor >= 0);

	// Set all values in the struct to 0 (Memory block can contain random data)
	memset(&tty, 0, sizeof tty);
	tcgetattr(fileDescriptor, &tty);

	// Set io baud rate (115200)
	cfsetospeed(&tty, (speed_t)B115200);
	cfsetispeed(&tty, (speed_t)B115200);

	tty.c_cflag     &=  ~PARENB;    	// No parity
	tty.c_cflag     &=  ~CSTOPB;		// No stop bit
	tty.c_cflag     &=  ~CSIZE;			// Clear and set character size (8 bits)
	tty.c_cflag     |=  CS8;
	tty.c_cflag     &=  ~CRTSCTS;		// No flow control
	tty.c_cflag     |=  CREAD | CLOCAL;	// Turn on READ & ignore ctrl lines

	tty.c_cc[VMIN]      =   1;          // Read doesn't block
	tty.c_cc[VTIME]     =   5;          // 0.5 seconds read timeout

	// Make data raw (no processing of data)
	cfmakeraw(&tty);

	// Flush Port, then applies attributes
	tcflush( fileDescriptor, TCIFLUSH );
	tcsetattr(fileDescriptor, TCSANOW, &tty);

	usleep(1000);

	/*
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
	unsigned char move[] = {0xFF, 0xFF, 0x0C, 0xFD, 0x06, 0xFE, 0x00, 0x3C, (pos & 0x00FF), (pos & 0xFF00) >> 8, (0x08 & 0xFD), 0xFD};

	send(statusError);
	sleep(1);
	send(torque);

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
	send(move);
	sleep(2);
	send(stat);

	unsigned char incomingBuffer[UART_BUFFER_SIZE];
	memset(incomingBuffer, 0, UART_BUFFER_SIZE);

	int received = read(fileDescriptor, incomingBuffer, UART_BUFFER_SIZE);
	incomingBuffer[received] = 0;
	cout << hex << incomingBuffer << dec << endl;*/
}

unsigned char* SerialControl::send(unsigned char command[])
{
	// Lock the mutex so that no other processes can call send()
	pthread_mutex_lock(&inUseMutex);

	// Calculate the checksums (bit 5 and 6)
	command[5] = calcCheck1(command);
	command[6] = calcCheck2(command[5]);

	// TODO test the calcchecksum method
	//calcChecksums(command);

	// Send the command
	int bytes = write(fileDescriptor, command, command[2]);
	if(bytes == -1) cout << errno << endl;

	// Print that a command has been sent
	cout << "Sent ";
	for(int i = 0; i < command[2]; i++)
		cout << hex << (int)command[i] << dec << " ";
	cout << endl;

//	int totalRead = 0;
//	int read = -1;
//	unsigned char incomingBuffer[UART_BUFFER_SIZE];

	// Unlock the mutex so that other processes can call send()
	pthread_mutex_unlock(&inUseMutex);

	return 0;
	//return incomingBuffer;
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

void SerialControl::calcChecksums(unsigned char buffer[])
{
	int packSize =  buffer[2];
	unsigned char checksum1 = (buffer[2] ^ buffer[3] ^ buffer[4]);

	if(packSize > 7)
		for(int i = 7; i < packSize; i++)
			checksum1 ^= buffer[i];

	buffer[5] = checksum1 &= 0xFE;
	buffer[6] = ~(checksum1) & 0xFE;
}
