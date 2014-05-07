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
	// Start the server
	Server server = new Server();
	server.start();

	// Delete the server instance
	delete(server);

	return 0;
}
