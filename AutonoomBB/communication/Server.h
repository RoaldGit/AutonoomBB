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
	static void *start(void * arg);
	void run();
private:
	int status,
		socketfd;
	struct addrinfo host_info;
	struct addrinfo *host_info_list;
};

#endif /* SERVER_H_ */
