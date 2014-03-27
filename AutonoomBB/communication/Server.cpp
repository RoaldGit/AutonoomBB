/*
 * Server.cpp
 *
 *  Created on: 13 mrt. 2014
 *      Author: Asus
 */

#include "Server.h"

#define NDEBUG

using namespace std;

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <assert.h>

Server::Server()
	:host_info_list(0)
{

}

Server::~Server()
{

}

void *Server::start(void * arg)
{
	cout << "Server thread started" << endl;
	return 0;
}

void Server::init()
{
	memset(&host_info, 0, sizeof host_info);

	cout << "Setting up the structs..." << endl;

	host_info.ai_flags = AI_PASSIVE;

	// NULL for localhost
	status = getaddrinfo(NULL, "5555", &host_info, &host_info_list);

	if (status != 0)
			cout << "getaddrinfo error" << gai_strerror(status) << endl;
	assert(status == 0);

	cout << "Creating socket..." << endl;
	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if(socketfd == -1)
		cout << "Socket creation error...." << endl;
	assert(socketfd != -1);

	cout << "Binding socket...." << endl;
	int yes = 1;
	status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if(status == -1)
		cout << "binding error..." << endl;
	assert(status != -1);

	run();
}

void Server::run()
{
	cout << "Listening for connections..."  << endl;
	status =  listen(socketfd, 5);
	if (status == -1)  cout << "listen error" << endl ;
	assert(status != -1);

	int new_sd;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);

	while(true)
	{
		new_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);

		if (new_sd == -1)
			cout << "listen error" << endl ;
		else
			cout << "Connection accepted. Using new_sd: "  <<  new_sd << "\t socketfd: " << socketfd << endl;

		assert(new_sd != -1);

		cout << "send()ing back a message..."  << endl;
		char *msg = "Connected.\n\rHello.\n\r";
		int len;
		ssize_t bytes_sent;
		len = strlen(msg);
		bytes_sent = send(new_sd, msg, len, 0);
		cout << "Message sent: " << msg << "\tBytes sent: " << bytes_sent << endl;

		freeaddrinfo(host_info_list);
		close(new_sd);
	}

	cout << "Stopping server..." << endl;
	close(socketfd);
}
