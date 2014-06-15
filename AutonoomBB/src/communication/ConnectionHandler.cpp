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
	cout << "Sending back a message..."  << endl;
	if(received == 0) cout << "host shut down" << endl;
	if(received == -1) cout << "recv error" << endl;

	// Add end string character for printing, and print received message
	dataBuffer[received] = '\0';
	cout << "Received " << received << " bytes." << endl;//|Message: " << endl << dataBuffer << endl << "------" << endl;

	// Find the start of the data (skip over all the headers)
	int start_body = findBody(dataBuffer);

	unsigned char *reply = 0;

	if(dataBuffer[start_body] == 'F')
		reply = handleSerialCommand(dataBuffer, start_body, received);
	else
		reply = handleTextualCommand(dataBuffer, start_body, received);

	// Send a reply
//	char *msg = "Message received.\n";
	int bytes_sent;
//	bytes_sent = send(socket, msg, strlen(msg), 0);'
	if(reply != 0)
	{
		bytes_sent = send(socket, reply, reply[2], 0);
	} else
	{
		char *no_reply = "No reply received.";
		bytes_sent = send(socket, no_reply, strlen(no_reply), 0);
		cout << "No reply could be read from UART. ";
	} cout << "Replied with " << bytes_sent << " bytes." << endl;

	//	cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

	// Close the socket descriptor
	close(socket);
}

/*
 * Handle a message only containing a serial command (FF FF 07 FD 07 00 00 = stat of Servo 253)
 */
unsigned char* ConnectionHandler::handleSerialCommand(char buffer[], int start_pos,  int end_pos)
{
	// Decode the message. The message, in case of serial data will be built up out of segments of 2 or 3 chars.
	// The last of these chars is a space, the rest represents a singe byte of the command
	unsigned char command[(end_pos - start_pos) / 3 + 1];

	int command_length = constructBytes(buffer, command, start_pos, end_pos);
	command[2] = command_length;

	SerialControl::getInstance()->send_calc_checksum(command);

	// TODO trim the command. The command array may contain garbage. Needs to be filtered out, maybe separate arrays
	// for each command to be sent separately or combined into a I/S_JOG command (advanced option). To separate the commands
	// look in the commands array at the 3rd byte (Command lenght), if a new command starts after those bytes (FF FF header)
	// a new command should follow.
	// EDIT: Double commands need to be filtered, because SerialControl::send() calculates a checksum. Additional
	// commands would be sent without a checksum == bad
	return 0;
}

unsigned char* ConnectionHandler::handleTextualCommand(char buffer[], int start_pos, int end_pos)
{
	string command = findCommand(buffer, start_pos, end_pos);

	unsigned char arguments[(start_pos + 5 - end_pos) / 3 + 1]; // start_pos + 5 because the command is 4 chars and a space
	int argument_length = constructBytes(buffer, arguments, start_pos + 5, end_pos);

	return SerialControl::getInstance()->send(arguments, command, argument_length);
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
			i += 2;
		}

		//i += 3;	// Each hex value in the message consists of 2 chars and is seperated with a space.
					// The next hex value starts 3 positions from the previous one
		bytes_constructed++;	// Increase the byte index
	}

	cout << endl << "Bytes extracted from message (" << bytes_constructed << " bytes): ";

//	for(int i = 0; i < bytes_constructed; i++)
//		printf("%x ", bytes[i]);
//	cout << endl;
	SerialControl::print_buffer(bytes, bytes_constructed);

	return bytes_constructed;
}

int ConnectionHandler::findCommandID(string command)
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
	else
		return 0;
}

int ConnectionHandler::findCommandID(char buffer[], int current_pos, int end_pos)
{
	string command = "";
	char current_char = buffer[current_pos];

	while(current_pos < end_pos && current_char != 0x20)
	{
		command += current_char;
		current_pos++;
		current_char = buffer[current_pos];
	} current_pos++; // Skip the space (0x20)

	return findCommandID(command);
}

string ConnectionHandler::findCommand(char buffer[], int current_pos, int end_pos)
{
	string command = "";
	char current_char = buffer[current_pos];

	while(current_pos < end_pos && current_char != 0x20)
	{
		command += current_char;
		current_pos++;
		current_char = buffer[current_pos];
	} current_pos++; // Skip the space (0x20)

	return command;
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
