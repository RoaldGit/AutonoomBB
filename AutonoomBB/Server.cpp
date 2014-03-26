/*
 * Server.cpp
 *
 *  Created on: 13 mrt. 2014
 *      Author: Asus
 */

#include "Server.h"

using namespace std;
#include <iostream>
#include <cstring>

Server::Server() {
	// TODO Auto-generated constructor stub

}

Server::~Server() {
	// TODO Auto-generated destructor stub
}

void Server::init() {
	memset(&host_info, 0, sizeof host_info);

	cout << "Setting up the structs..." << endl;

	host_info.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, "5555", &host_info, &host_info_list);

	if (status != 0)
			cout << "getaddrinfo error" << gai_strerror(status) << endl;

	cout << "Creating socket..." << endl;

	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if(socketfd == -1)
		cout << "Socket creation error...." << endl;

	cout << "Binding socket...." << endl;
	int yes = 1;
	status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if(status == -1)
		cout << "binding error..." << endl;

	std::cout << "Listening for connections..."  << std::endl;
	status =  listen(socketfd, 5);
	if (status == -1)  std::cout << "listen error" << std::endl ;
}

