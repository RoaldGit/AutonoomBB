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
	char unsigned dataBuffer[TCP_BUFFER_SIZE];

	// Receive message
	received = recv(socket, dataBuffer, TCP_BUFFER_SIZE, 0);
	cout << "sending back a message..."  << endl;
	if(received == 0) cout << "host shut down" << endl;
	if(received == -1) cout << "recv error" << endl;

	// Add end string character for printing, and print received message
	dataBuffer[received] = '\0';
	cout << "Received " << received << " bytes|Message: " << endl << dataBuffer << endl << "------" << endl;
	for(int i = 0; i < received; i++)
	{
		printf("%x ", dataBuffer[i]);
		if(dataBuffer[i] == '\n')
			cout << endl;
	}

	cout << endl;

	// Send a reply
	char *msg = "Connected.\n";
	ssize_t bytes_sent;
	bytes_sent = send(socket, msg, strlen(msg), 0);
	cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

	// Close the socket descriptor
	close(socket);
}

int ConnectionHandler::findBody(unsigned char buffer[], int length)
{
	int body_start = 0;
	while(body_start + 3 < length)
	{
		if(buffer[body_start] == '\r' && buffer[body_start + 1] == '\n' && buffer[body_start + 2] == '\r' && buffer[body_start + 3] == '\n')
			return body_start + 4;
		body_start++;
	}
	return 0;
}
