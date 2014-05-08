/*
 * SerialControl.h
 *
 *  Created on: 8 mei 2014
 *      Author: Asus
 */

#ifndef SERIALCONTROL_H_
#define SERIALCONTROL_H_

#define BAUDRATE B115200			// Baudrate for serial communication, bps
#define SERIALDEVICE "/dev/ttyO4"	// Use UART4
#define UART_BUFFER_SIZE 255
#define TRUE 1
#define FALSE 0

class SerialControl {
public:
	SerialControl();
	virtual ~SerialControl();
	pthread_t serialThread;
private:
	int fileDescriptor;		// Pointer to the device file

};

#endif /* SERIALCONTROL_H_ */
