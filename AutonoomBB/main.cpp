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
	// Create a server instance
	Server *server = new Server();

	// Create a POSIX Thread instance for the server
	pthread_t serverThread;

	// Start the server thread by calling the start() function, an empty void* function required
	// for the use of pthread_create()
	pthread_create(&serverThread, NULL, server->start, server);
	pthread_join(serverThread, NULL);

	// Setup the server
	((Server *)serverThread)->init();

	// Server is setup
	cout << "done" << endl;

	// Delete the server instance
	delete(server);

	return 0;
}
