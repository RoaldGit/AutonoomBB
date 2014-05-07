/*
 * Server.cpp
 *
 *  Created on: 13 mrt. 2014
 *  Author: Roald
 *  Based on http://codebase.eu/tutorial/linux-socket-programming-c
 */

#include "Server.h"

#define NDEBUG
#define BUFFER_SIZE 1024

using namespace std;

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <assert.h>

// Constructor, initialize all private variables
Server::Server()
	:status(0), socketfd(0), host_info_list(0)
{

}

// Destructor, remove all created resources
Server::~Server()
{

}

// Start function, required for the use of pthread_create
void *Server::start(void *)
{
	cout << "Server thread started" << endl;
	return 0;
}

// Setup the server
void Server::init()
{
	// Set all values in the struct to 0 (Memory block can contain random data)
	memset(&host_info, 0, sizeof host_info);

	cout << "Setting up the structs..." << endl;

	// Allow IPv4 and IPv6
	host_info.ai_family = AF_UNSPEC;
	// TCP
	host_info.ai_socktype = SOCK_STREAM;
	// IP wildcard, enables bind() and accept()
	host_info.ai_flags = AI_PASSIVE;

	// Retrieve address info for localhost:5555 (IP is NULL for localhost)
	status = getaddrinfo(NULL, "5555", &host_info, &host_info_list);
	if (status != 0) cout << "getaddrinfo error" << gai_strerror(status) << endl;
			assert(status == 0);

	cout << "Creating socket..." << endl;

	// Create the socket
	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if(socketfd == -1) cout << "Socket creation error...." << endl;
		assert(socketfd != -1);

	cout << "Binding socket...." << endl;

	// Allow the reuse of local addresses
	int yes = 1;
	status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if(status == -1) cout << "binding error..." << endl;
		assert(status != -1);

	// Start listening for connections
	run();
}

// Listen for incoming connections
void Server::run()
{
	cout << "Listening for connections..."  << endl;

	// Start listening for connections
	status =  listen(socketfd, 5);
	if (status == -1)  cout << "listen error" << endl ;
		assert(status != -1);

	// Socket descriptor for incoming connection
	int newSocket;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);

	while(true)
	{
		// Timeout value for recv()
		struct timeval timeoutValue;
		timeoutValue.tv_sec = 30;

		// Accept incoming connection and set timeout value (RCVTIMEO)
		newSocket = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
		status = setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutValue, sizeof(struct timeval));
		if (newSocket == -1) cout << "listen error" << endl;
			assert(newSocket != -1);

		// Bytes received and data buffer
		ssize_t received;
		char dataBuffer[BUFFER_SIZE];

		// Receive message
		received = recv(newSocket, dataBuffer, BUFFER_SIZE, 0);
		cout << "send()ing back a message..."  << endl;
		if(received == 0) cout << "host shut down" << endl;
		if(received == -1) cout << "recv error" << endl;

		// Add end string caracter for printing, and print received message
		dataBuffer[received] = '\0';
		cout << "Received " << received << " bytes|Message: " << dataBuffer << endl;

		// Send a reply
		char *msg = "Connected.\n";
		ssize_t bytes_sent;
		bytes_sent = send(newSocket, msg, strlen(msg), 0);
		cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

		// Close the socket descriptor
		close(newSocket);
	}

	cout << "Stopping server..." << endl;

	// Close the socket descriptor and free the memory used by the host info list
	freeaddrinfo(host_info_list);
	close(socketfd);
}
