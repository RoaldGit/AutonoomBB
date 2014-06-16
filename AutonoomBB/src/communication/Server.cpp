/*
 * Server.cpp
 *
 *  Created on: 13 mrt. 2014
 *  Author: Roald
 *  Based on http://codebase.eu/tutorial/linux-socket-programming-c
 */

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>

#include "Server.h"
#include "ConnectionHandler.h"

#define NDEBUG
#define TCP_BUFFER_SIZE 1024
#define FALSE 0
#define TRUE 1
#define SOCKET_TIMEOUT 5

using namespace std;

// Constructor, initialize all private variables
Server::Server()
	:status(0), listenSocket(0), running(false), stopRequested(false),
	 serverThread(0), host_info_list(0)
{

}

// Destructor, remove all created resources
Server::~Server()
{

}

// Start the server
pthread_t Server::start()
{
	assert(running == false);
	running = true;

	// Initialize the Thread
	pthread_create(&serverThread, NULL, &Server::startUp, this);
	cout << "Server thread started" << endl;

	// Return the thread
	return serverThread;
}

// Startup method, calls the init method of the server
void *Server::startUp(void *obj)
{
	// pthread_create passes the server reference (this) to the startup method. Cast to Server so that the init method
	// can be called
	Server *server = reinterpret_cast<Server *>(obj);
	server->init();

	return 0;
}


// Stop the server
void Server::stop()
{
	// Server must be running
	assert(running == true);

	cout << "Stopping server..." << endl;

	// Terminate while loop in the run method
	stopRequested = true;

	// Wait for the run method to end
	pthread_join(serverThread, NULL);
	cout << "Server stopped" << endl;
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
	listenSocket = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if(listenSocket == -1) cout << "Socket creation error...." << endl;
		assert(listenSocket != -1);

	cout << "Binding socket...." << endl;

	// Allow the reuse of local addresses
	int yes = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	status = bind(listenSocket, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if(status == -1) cout << "binding error..." << endl;
		assert(status != -1);

	// Start listening for connections
	run();
}

// Listen for incoming connections
void Server::run()
{
	// Timeout value
	struct timeval timeoutValue;
	timeoutValue.tv_sec = SOCKET_TIMEOUT;

	// Set for select()
	fd_set sockets;

	// Activity

	cout << "Listening for connections..."  << endl;

	// Start listening for connections
	status =  listen(listenSocket, 5);
	if (status == -1)  cout << "listen error" << endl ;
		assert(status != -1);

	// Socket descriptor for incoming connection
	int newSocket;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);

	while(!stopRequested)
	{
		// Clear the socket set and add the listen socket to the set
		FD_ZERO(&sockets);
		FD_SET(listenSocket, &sockets);

		select(listenSocket + 1, &sockets, NULL, NULL, &timeoutValue);
		if(FD_ISSET(listenSocket, &sockets))
		{
		// Accept incoming connection and set timeout value (RCVTIMEO)
		newSocket = accept(listenSocket, (struct sockaddr *)&their_addr, &addr_size);
		status = setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutValue, sizeof(struct timeval));

		if (newSocket == -1) cout << "Accept returned " << errno << endl;
		else
		{
			ConnectionHandler* newConnection = new ConnectionHandler(newSocket);
			newConnection->start();
		}}
	}

	// Close the socket descriptor and free the memory used by the host info list
	freeaddrinfo(host_info_list);
	close(listenSocket);
}
