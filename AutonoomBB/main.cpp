//============================================================================
// Name        : main.cpp
// Author      :
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "communication/server.h"

using namespace std;

int main() {
	// Start the server
	Server *server = new Server();
	pthread_t serverThread = server->start();

	// Wait for the Thread to finish
	pthread_join(serverThread, NULL);

	// Delete the server instance
	delete(server);

	cout << "Done" << endl;

	return 0;
}
