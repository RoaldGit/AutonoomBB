//============================================================================
// Name        : main.cpp
// Author      :
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include "Server.h"
using namespace std;

int main() {
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	int status;
	struct addrinfo host_info;
	struct addrinfo *host_info_list;

	memset(&host_info, 0, sizeof host_info);

	cout << "Setting up the structs..." << endl;

	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo("192.168.7.1","80", &host_info, &host_info_list);
	if (status != 0)
		cout << "getaddrinfo error" << gai_strerror(status) << endl;

	cout << "Creating sockdet..." << endl;
	int socketfd;
	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if(socketfd == -1)
		cout << "Socket error...." << endl;

	cout << "Connecting to 192.168.7.1" << endl;
	status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if(status == -1)
		cout << "connect error..." << endl;

	cout << "sending message...." << std::endl;
	char *msg = "hahaha";
	int len = strlen(msg);
	send(socketfd, msg, len, 0);

	return 0;
}
