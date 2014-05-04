//============================================================================
// Name        : main.cpp
// Author      :
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include "communication/server.h"
#include <pthread.h>
#include <unistd.h>

using namespace std;

int main() {
	Server *server = new Server();
	pthread_t serverThread;

	pthread_create(&serverThread, NULL, server->start, server);
	pthread_join(serverThread, NULL);
	((Server *)serverThread)->init();

	cout << "done" << endl;
	///server->init();

	delete(server);

//	cout << "Connecting to 192.168.7.1" << endl;
//	status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
//	if(status == -1)
//		cout << "connect error..." << endl;
//
//	cout << "sending message...." << std::endl;
//	char *msg = "hahaha";
//	int len = strlen(msg);
//	send(socketfd, msg, len, 0);

	return 0;
}
