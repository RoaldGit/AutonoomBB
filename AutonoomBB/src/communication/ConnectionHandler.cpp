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

//#include "util/TCP.h"

using namespace std;

#define TCP_BUFFER_SIZE 1024

// TODO TCP packet struct for handling

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

/*
	// print message as hex, for debug purposes
	for(int i = 0; i < received; i++)
	{
		printf("%x ", dataBuffer[i]);
		if(dataBuffer[i] == '\n')
			cout << endl;
	} cout << endl;*/

	// Decode the message. The message, in case of serial data will be built up out of segments of 3 chars.
	// 2 of these chars represent a hexidecimal value (FF), the 3rd is a space.
	unsigned char commands[1000];
	unsigned char chars[2];
	int bytes_made = 0;

	for(int i = start_body; i < received;)
	{
		chars[0] = dataBuffer[i];
		chars[1] = dataBuffer[i + 1];

		for(int k = 0; k < 2; k++)
		{
			if(chars[k] > 40)
				chars[k] = chars[k] - 1;

			chars[k] = chars[k] - 30;
		}

		chars[0] = chars[0] << 4;
		commands[bytes_made] = chars[0] & chars[1];
	}

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
