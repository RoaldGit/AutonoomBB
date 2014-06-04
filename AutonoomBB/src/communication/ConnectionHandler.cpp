/*
 * ConnectionHandler.cpp
 *
 *  Created on: 23 mei 2014
 *      Author: Asus
 */

#include <pthread.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <iomanip>
#include <cstdio>

#include "ConnectionHandler.h"
#include "SerialControl.h"
#include "../util/SerialCommand.h"

using namespace std;

#define TCP_BUFFER_SIZE 1024

ConnectionHandler::ConnectionHandler(int socket)
	:socket(socket)
{

}

ConnectionHandler::~ConnectionHandler()
{

}

void ConnectionHandler::start()
{
	pthread_t connectionThread;
	pthread_create(&connectionThread, NULL, &ConnectionHandler::startup, this);
}

void *ConnectionHandler::startup(void *obj)
{
	ConnectionHandler *connection = reinterpret_cast<ConnectionHandler *>(obj);
	connection->handleConnection();

	return 0;
}

void ConnectionHandler::handleConnection()
{
	// Bytes received and data buffer
	ssize_t received;
	char dataBuffer[TCP_BUFFER_SIZE];

	// Receive message
	received = recv(socket, dataBuffer, TCP_BUFFER_SIZE, 0);
	cout << "sending back a message..."  << endl;
	if(received == 0) cout << "host shut down" << endl;
	if(received == -1) cout << "recv error" << endl;

	// Add end string character for printing, and print received message
	dataBuffer[received] = '\0';
	cout << "Received " << received << " bytes." << endl;//|Message: " << endl << dataBuffer << endl << "------" << endl;

	// Find the start of the data (skip over all the headers)
	int start_body = findBody(dataBuffer);

	if(dataBuffer[start_body] == 'F')
		handleSerialCommand(dataBuffer, start_body, received);
	else
		handleTextualCommand(dataBuffer, start_body, received);

//	unsigned char *command = status;
//	command[3] = 0x0A;
//
//	SerialControl::getInstance()->send(command);

	// Send a reply
	char *msg = "Message received.\n";
	ssize_t bytes_sent;
	bytes_sent = send(socket, msg, strlen(msg), 0);
	cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

	// Close the socket descriptor
	close(socket);
}

/*
 * Handle a message only containing a serial command (FF FF 07 FD 07 00 00 = stat of Servo 253)
 */
void ConnectionHandler::handleSerialCommand(char buffer[], int start_pos,  int end_pos)
{
	// Decode the message. The message, in case of serial data will be built up out of segments of 3 chars.
	// 2 of these chars represent a hexidecimal value (FF), the 3rd is a space.
	unsigned char commands[(end_pos - start_pos) / 3 + 1];	// Filtered command
	/*int chars[2];										// 2 bytes making the actual hex value
	int bytes_made = 0;											// Number of hex values made/extracted so far, index for commands[]

	for(int i = start_pos; i < end_pos;)
	{
		// Read 2 characters from the data buffer
		chars[0] = buffer[i];
		chars[1] = buffer[i + 1];

		// Substract 48 from both chars, because ASCII - 48 is hex value when working with uppercase letters.
		// Substract 7 due to some characters between 9 an A
		for(int k = 0; k < 2; k++)
		{
			if(chars[k] > 58)
				chars[k] = chars[k] - 7;

			chars[k] = chars[k] - 48;
		}

		// Shift the high bit by 4, turning 0x0F into 0xF0
		if(chars[1] > 0)
		{
			chars[0] = chars[0] << 4;
			commands[bytes_made] = chars[0] + chars[1];
		} else
			commands[bytes_made] = chars[0];

		// Bitwise AND to combine the bits, forming a single hex value

		i += 3;			// Each hex value in the message consists of 2 chars and is seperated with a space.
						// The next hex value starts 3 positions from the previous one
		bytes_made++;	// Increase the command index
	}

	// DEBUG
	cout << "Command extracted from message: ";

	for(int i = 0; i < bytes_made; i++)
	{
		printf("%x ", commands[i]);
	} cout << endl;*/

	constructBytes(buffer, commands, start_pos, end_pos);

	SerialControl::getInstance()->send(commands);

	// TODO trim the command. The command array may contain garbage. Needs to be filtered out, maybe separate arrays
	// for each command to be sent separately or combined into a I/S_JOG command (advanced option). To separate the commands
	// look in the commands array at the 3rd byte (Command lenght), if a new command starts after those bytes (FF FF header)
	// a new command should follow.
	// EDIT: Double commands need to be filtered, because SerialControl::send() calculates a checksum. Additional
	// commands would be sent without a checksum == bad
}

void ConnectionHandler::handleTextualCommand(char buffer[], int start_pos, int end_pos)
{
	int current_pos = start_pos;
	string command = "";
	char current_char = buffer[current_pos];

	while(current_pos < end_pos && current_char != 0x20)
	{
		command += current_char;
		current_pos++;
		current_char = buffer[current_pos];
	} current_pos++; // Skip the space (0x20)

	//	cout << command << endl;
	unsigned char arguments[(current_pos - end_pos) / 3 + 1];
	int argument_length = constructBytes(buffer, arguments, current_pos, end_pos);
	int command_length =  argument_length + 6;

	cout << command_length << endl;
	unsigned char bytes[command_length];

	bytes[0] = 0xFF;
	bytes[1] = 0xFF;
	// Set packet length
	bytes[2] = command_length;
	// Set address
	bytes[3] = arguments[0];
	// Set command ID
	bytes[4] = findCommandID(command);

	// Add the remaining arguments
	for(int i = 7; i < command_length; i++)
		bytes[i] = arguments[i - 6];

	SerialControl::getInstance()->send(bytes);
}

int ConnectionHandler::constructBytes(char buffer[], unsigned char bytes[], int start_pos, int end_pos)
{
	int chars[2];
	int bytes_constructed = 0;

	for(int i = start_pos; i < end_pos;)
	{
		// Read 2 characters from the data buffer
		chars[0] = buffer[i];
		chars[1] = buffer[i + 1];

		// Substract 48 from both chars, because ASCII - 48 is hex value when working with uppercase letters.
		// Substract 7 due to some characters between 9 an A
		for(int k = 0; k < 2; k++)
		{
			if(chars[k] > 58)
				chars[k] = chars[k] - 7;

			chars[k] = chars[k] - 48;
		}

		// Check if double chars were received. This way, you can send either 0A or A. The result should be the same
		if(chars[1] >= 0)
		{
			// Shift the high bit by 4, turning 0x0F into 0xF0 and add up the two parts of the byte
			chars[0] = chars[0] << 4;
			bytes[bytes_constructed] = chars[0] + chars[1];
			i += 3; // Two byte and a space, so you need to read the next character 2 spaces from the current i
		} else
		{
			bytes[bytes_constructed] = chars[0];
			i += 2; //
		}

		//i += 3;	// Each hex value in the message consists of 2 chars and is seperated with a space.
					// The next hex value starts 3 positions from the previous one
		bytes_constructed++;	// Increase the command index
	}

	cout << "Bytes extracted from message (" << bytes_constructed << " bytes): ";

	for(int i = 0; i < bytes_constructed; i++)
	{
		printf("%x ", bytes[i]);
	} cout << endl;

	return bytes_constructed;
}

int ConnectionHandler::findCommandID(string command)
{
	if(command == "status")
		return 7;
	else if(command == "led")
		return 3;
	else
		return 0;
}

int ConnectionHandler::findBody(char buffer[])
{
	// HTML headers are closed by a double carriage return and newline
	char end[] = "\r\n\r\n";
	// Find the end of the HTML header

	if(strstr(buffer, end) == NULL)
		return 0;
	else
		return strstr(buffer, end) - buffer + 4;
}
