/*
 * ConnectionHandler.cpp
 *
 *  Created on: 23 mei 2014
 *      Author: Asus
 */

#include "ConnectionHandler.h"

#include <pthread.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <iomanip>

#include <cstdio>
#include <string>

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
	cout << "Received " << received << " bytes|Message: " << endl << dataBuffer << endl << "------" << endl;

	// Find the start of the data (skip over all the headers)
	int start_body = findBody(dataBuffer);

	// print message as hex, for debug purposes
//	for(int i = 0; i < received; i++)
//	{
//		printf("%x ", dataBuffer[i]);
//		if(dataBuffer[i] == '\n')
//			cout << endl;
//	} cout << endl;

	// Decode the message. The message, in case of serial data will be built up out of segments of 3 chars.
	// 2 of these chars represent a hexidecimal value (FF), the 3rd is a space.
	unsigned char commands[(received - start_body) / 3 + 1];	// Filtered command
	unsigned char chars[2];										// 2 bytes making the actual hex value
	int bytes_made = 0;											// Number of hex values made/extracted so far, index for commands[]

	for(int i = start_body; i < received;)
	{
		// Read 2 characters from the data buffer
		chars[0] = dataBuffer[i];
		chars[1] = dataBuffer[i + 1];

		// Substract 30 from both chars, because ASCII - 30 is hex value when working with uppercase letters.
		// Because ASCII 40 is
		for(int k = 0; k < 2; k++)
		{
			if(chars[k] > 40)
				chars[k] = chars[k] - 1;

			chars[k] = chars[k] - 30;
			// in case the above doesn't work (It counts on databuffer containing hex values, not ascii values)
//			if(chars[k] > 58)
//				chars[k] = chars[k] - 7;
//
//			chars[k] = chars[k] - 48;
		}

		// Shift the high bit by 4, turning 0x0F into 0xF0
		chars[0] = chars[0] << 4;
		// Bitwise AND to combine the bits, forming a single hex value
		commands[bytes_made] = chars[0] & chars[1];

		i += 3;			// Each hex value in the message consists of 2 chars and is seperated with a space.
						// The next hex value starts 3 positions from the previous one
		bytes_made++;	// Increase the command index
	}

	// DEBUG
	commands[bytes_made] = '\0'; // close the command array for printing
	cout << "Command extracted from message: ";

	for(int i = 0; i < bytes_made; i++)
	{
		printf("%x ", commands[i]);
	} cout << endl;
	//

	// TODO trim the command. The command array may contain garbage. Needs to be filtered out, maybe separate arrays
	// for each command to be sent separately or combined into a I/S_JOG command (advanced option). To separate the commands
	// look in the commands array at the 3rd byte (Command lenght), if a new command starts after those bytes (FF FF header)
	// a new command should follow.
	// EDIT: Double commands need to be filtered, because SerialControl::send() calculates a checksum. Additional
	// commands would be sent without a checksum == bad

	//Uncomment when the command filter is correct. No need
	//SerialControl::getInstance() -> send(commands);

	// Send a reply
	char *msg = "Message received.\n";
	ssize_t bytes_sent;
	bytes_sent = send(socket, msg, strlen(msg), 0);
	cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

	// Close the socket descriptor
	close(socket);
}


int ConnectionHandler::findBody(char buffer[])
{
	// HTML headers are closed by a double carriage return and newline
	char end[] = "\r\n\r\n";
	// Find the end of the HTML header
	return strstr(buffer, end) - buffer + 4;
}
