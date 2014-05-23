/*
 * ConnectionHandler.cpp
 *
 *  Created on: 23 mei 2014
 *      Author: Asus
 */

#include "ConnectionHandler.h"

#include <pthread.h>

using namespace std;


ConnectionHandler::ConnectionHandler(int socket)
	:socket(socket)
{

}

ConnectionHandler::~ConnectionHandler()
{

}

void ConnectionHandler::handleConnection(int socket)
{
	ConnectionHandler newConnection = new ConnectionHandler(socket);
}
