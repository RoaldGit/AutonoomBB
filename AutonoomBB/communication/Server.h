/*
 * Server.h
 *
 *  Created on: 13 mrt. 2014
 *      Author: Asus
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <netdb.h>

class Server {
public:
	Server();
	virtual ~Server();
	void init();
	static void *start(void *);
	void run();
private:
	int status,			// Used as storage for return values of functions
		socketfd;		// The socket descriptor

	// The struct that will contain the addrinfo retreived from getaddrinfo()
	struct addrinfo host_info;
	// Pointer to the host info list
	struct addrinfo *host_info_list;
};

#endif /* SERVER_H_ */
