/*
 * ConnectionHandler.h
 *
 *  Created on: 23 mei 2014
 *      Author: Asus
 */

#ifndef CONNECTIONHANDLER_H_
#define CONNECTIONHANDLER_H_

class ConnectionHandler {
public:
	ConnectionHandler(int socket);
	virtual ~ConnectionHandler();
	void start();

private:
	static void* startup(void *);
	void handleConnection();
	int findBody(char buffer[]);
	unsigned char constructByte();
	int socket;					// Socket where the data will come in
};

#endif /* CONNECTIONHANDLER_H_ */
