/*
 * Server.h
 *
 *  Created on: 13 mrt. 2014
 *      Author: Asus
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <netdb.h>
#include <pthread.h>

class Server {
public:
	Server();
	virtual ~Server();
	pthread_t start();
	void stop();

private:
	int status,					// Used as storage for return values of functions
		listenSocket;			// The socket descriptor
	bool running,				// Is the server running
		stopRequested;			// Should the server stop
	pthread_t serverThread;		// Thread

	// The struct that will contain the addrinfo retreived from getaddrinfo()
	struct addrinfo host_info;
	// Pointer to the host info list
	struct addrinfo *host_info_list;

	static void *startUp(void *);	// Startup method for Thread
	void init();					// Setup the server
	void run();						// Run the server (Listen for connections)
};

#endif /* SERVER_H_ */
