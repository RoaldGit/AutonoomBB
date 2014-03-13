/*
 * Server.h
 *
 *  Created on: 13 mrt. 2014
 *      Author: Asus
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>

class Server {
public:
	Server();
	virtual ~Server();
};

#endif /* SERVER_H_ */
