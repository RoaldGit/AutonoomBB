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
	void handleSerialCommand(char buffer[], int start_pos, int end_pos);
	void handleTextualCommand(char buffer[], int start_pos, int end_pos);
	int constructBytes(char buffer[], unsigned char bytes[], int start_pos, int end_pos);
	int findBody(char buffer[]);

	int socket;					// Socket where the data will come in
};

#endif /* CONNECTIONHANDLER_H_ */
