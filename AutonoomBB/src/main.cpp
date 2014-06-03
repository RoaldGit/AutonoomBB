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

#include "communication/Server.h"
#include "communication/SerialControl.h"

using namespace std;

int main() {
	// Start the server
	Server *server = new Server();
	pthread_t serverThread = server->start();

	SerialControl *serialTest = SerialControl::getInstance();
	serialTest->setup();

	//sleep(20);
	//server->stop();

	// Wait for the Thread to finish
	pthread_join(serverThread, NULL);

	// Delete the server instance
	delete(server);

	cout << "Done" << endl;

	return 0;
}
