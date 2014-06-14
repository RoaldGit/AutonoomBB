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
#include <sys/ioctl.h>

#include "SerialControl.h"

using namespace std;

#define SERIAL_DEBUG = true;

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
	flush_serial_port();
	tcsetattr(fileDescriptor, TCSANOW, &tty);

	usleep(1000);
}

unsigned char* SerialControl::send(unsigned char command[], int length)
{
	pthread_mutex_lock(&inUseMutex);

	int bytes_sent = write(fileDescriptor, command, length);
	if(bytes_sent == -1) cout << errno << endl;

	#ifdef SERIAL_DEBUG
		print_buffer(command, length);
	#endif

	//TODO Read
	return 0;
}

unsigned char* SerialControl::send_calc_checksum(unsigned char command[])
{
	// Lock the mutex so that no other processes can call send()
	pthread_mutex_lock(&inUseMutex);

	// Calculate the checksums for the command
	calc_checksums(command);

	// Send the command
	int bytes = write(fileDescriptor, command, command[2]);
	if(bytes == -1) cout << errno << endl;

	// Print that a command has been sent
	cout << "Sent " << bytes << " bytes: ";
	for(int i = 0; i < command[2]; i++)
		cout << hex << (int)command[i] << dec << " ";
	cout << endl;

	// TODO Read

	// Unlock the mutex so that other processes can call send()
	pthread_mutex_unlock(&inUseMutex);

	return 0;
	//return incomingBuffer;
}

unsigned char* SerialControl::send(unsigned char buffer[], string command, int argument_length)
{
	// Get the command_id using the string filtered from the message
	int command_id = find_command_id(command);

	switch(command_id)
	{
		case 0: return 0;
//		case 1: return send_eepw(buffer, argument_length);
//		case 2: return send_eepr(buffer, argument_length);
//		case 3: return send_ramw(buffer, argument_length);
//		case 4: return send_ramr(buffer, argument_length);
//		case 5: return send_ijog(buffer);
		case 6: return send_sjog(buffer, argument_length);
//		case 7: return send_stat(buffer, argument_length);
//		case 8: return send_roll(buffer);
//		case 9: return send_boot(buffer);
		default: send_command(command_id, buffer, argument_length);
	}

	// TODO Read
	unsigned char reply[UART_BUFFER_SIZE];
	memset(reply, 0, UART_BUFFER_SIZE);

	int bytes_read = read(fileDescriptor, reply, UART_BUFFER_SIZE);
//	int bytes_read = 0;
	int bytes_available = 0;
//	while(ioctl(fileDescriptor, FIONREAD, bytes_available) )
	int tries = 1;

	usleep(100);
	ioctl(fileDescriptor, FIONREAD, &bytes_available);
			cout << "Bytes available: " << bytes_available << endl;
	while(bytes_read == -1 && tries < 10)
	{
		ioctl(fileDescriptor, FIONREAD, &bytes_available);
		cout << "Bytes available: " << bytes_available << endl;
		//bytes_read = read(fileDescriptor, reply, UART_BUFFER_SIZE);
		tries++;
	}


	cout << "Received " << bytes_read << " bytes (" << tries << " tries) : ";

	print_buffer(reply, bytes_read);

	return 0;
}

int SerialControl::find_command_id(string command)
{
	if(command == "eepw")
		return 1;
	else if(command == "eepr")
		return 2;
	else if(command == "ramw")
		return 3;
	else if(command == "ramr")
		return 4;
	else if(command == "ijog")
		return 5;
	else if(command == "sjog")
		return 6;
	else if(command == "stat")
		return 7;
	else if(command == "roll")
		return 8;
	else if(command == "boot")
		return 9;
	else if(command == "flsh")
		flush_serial_port();
	else
		return 0;
	return 0;
}

unsigned char* SerialControl::send_command(int command_id, unsigned char arguments[], int argument_length)
{
	int command_length =  argument_length + 6;
	unsigned char bytes[command_length];
	// Set header
	bytes[0] = 0xFF;
	bytes[1] = 0xFF;
	// Set packet length
	bytes[2] = command_length;
	// Set address
	bytes[3] = arguments[0];
	// Set command ID
	bytes[4] = command_id;
	// Add the remaining arguments
	for(int i = 7; i < command_length; i++)
		bytes[i] = arguments[i - 6];

	return send_calc_checksum(bytes);
}

unsigned char* SerialControl::send_sjog(unsigned char arguments[], int argument_length)
{
	return 0;
}

void SerialControl::calc_checksums(unsigned char buffer[])
{
	int packSize =  buffer[2];
	unsigned char checksum1 = (buffer[2] ^ buffer[3] ^ buffer[4]);

	if(packSize > 7)
		for(int i = 7; i < packSize; i++)
			checksum1 ^= buffer[i];

	buffer[5] = checksum1 &= 0xFE;
	buffer[6] = ~(checksum1) & 0xFE;
}

void SerialControl::print_buffer(unsigned char buffer[], int length)
{
	for(int i = 0; i < length; i++)
		cout << hex << (int)buffer[i] << dec << " ";
	cout << endl;
}

void SerialControl::flush_serial_port()
{
	// Flush the serial port

	#ifdef SERIAL_DEBUG
	cout << "Flushing serial port" << endl;
	#endif

	tcflush( fileDescriptor, TCIFLUSH );
}
