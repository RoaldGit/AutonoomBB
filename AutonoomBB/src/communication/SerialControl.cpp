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

#define SERIAL_DEBUG true
#define SERIAL_READ_TIMEOUT 10000

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
	#ifdef SERIAL_DEBUG
	cout << "Sent " << bytes << " bytes: ";
	print_buffer(command, command[2]);
	#endif

	// TODO Read

	// Unlock the mutex so that other processes can call send()
	pthread_mutex_unlock(&inUseMutex);

	return 0;
	//return incomingBuffer;
}

unsigned char* SerialControl::send(unsigned char arguments[], string command, int argument_length)
{
	// Get the command_id using the string filtered from the message
	int command_id = find_command_id(command);
	int expected_reply_size = 0;

	switch(command_id)
	{
		case 0: return 0;
		case 5: return send_ijog(arguments, argument_length);
		case 6: return send_sjog(arguments, argument_length);
		case 7: expected_reply_size = 9; // Default bytes such as header, +2 status bytes
				send_command(command_id, arguments, argument_length);
				break;
		default:expected_reply_size = 11 + arguments[2]; // Same as case 7, +2 for register and length, + number of bytes requested
				send_command(command_id, arguments, argument_length);
				break;
	}

	return read_serial(expected_reply_size);
}

unsigned char* SerialControl::read_serial(int bytes_expected)
{
	int bytes_available = 0;
	int bytes_read = 0;
	int timeout = 0;

	while(bytes_available < bytes_expected && timeout < SERIAL_READ_TIMEOUT)
	{
		ioctl(fileDescriptor, FIONREAD, &bytes_available);
		usleep(1);
		timeout++;
	}

	unsigned char reply[bytes_available];
	memset(reply, 0, bytes_available);

	bytes_read = read(fileDescriptor, reply, bytes_available);

	#ifdef SERIAL_DEBUG
	cout << "Bytes available: " << bytes_available << "|Bytes expected: " << bytes_expected << "|Read " << bytes_read << " bytes : ";
	print_buffer(reply, bytes_read);
	#endif

	return reply;
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

unsigned char* SerialControl::send_ijog(unsigned char arguments[], int argument_length)
{
	return 0;
}

unsigned char* SerialControl::send_sjog(unsigned char arguments[], int argument_length)
{
	/*	SJOG commands are 12 bytes long FF FF C X 6 S S P G G L X
	 * FF FF header
	 * C is length
	 * X is address
	 * 6 is SJOG command
	 * S S are the checksums
	 * P is playtime
	 * G G is goal pos (LSB and MSB)
	 * L is set (Mostly used for led)
	 *
	 * arguments[] contains the arguments in the following format:
	 * P G G L X
	 * P is playtime
	 * G G is goal pos (LSB and MSB)
	 * L is set
	 * X is address
	 * G G L X can be repeated up to 53 times according to the HerkuleX docs.
	 * This function will split the incoming arguments into seperate packets, because the servo's apparently can't
	 * recognize the combined command. This is suspected to be due to the command being for the official
	 * servo controller of Dongbu
	 */
	unsigned char bytes[12];
	/*
	 * Playtime is used in every command, so it doesn't count towards the number of packets to send.
	 * Each packet is 4 arguments
	 */
	int packets = (argument_length - 1) / 4;
	int playtime = arguments[0];
//	int argument_set = packets * 4;

	cout << "Sending " << packets << " SJOG packets" << endl;

	for(int i = 0; i < packets; i++)
	{
		// Set header
		bytes[0] = 0xFF;
		bytes[1] = 0xFF;
		// Set packet length
		bytes[2] = 12;
		// Set address
		bytes[3] = arguments[4 + i * 4];
		// Set command ID
		bytes[4] = 6;
		bytes[7] = playtime;
		// Set the remaining arguments (LSB, MSB, SET and Address)
		for(int k = 1; k < 5; k++)
			bytes[7 + k] = arguments[k + i * 4];

		send_calc_checksum(bytes);
	}

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
	#ifdef SERIAL_DEBUG
	cout << "Flushing serial port" << endl;
	#endif

	// Flush the serial port
	tcflush( fileDescriptor, TCIFLUSH );
}
