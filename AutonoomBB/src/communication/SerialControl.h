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
	virtual ~SerialControl();
	static SerialControl* getInstance();
	void restoreDefault();
	void setup();
private:
	SerialControl();
	int fileDescriptor;		// Pointer to the device file
	pthread_t serialThread;	// Thread controlling serial trafic
	pthread_mutex_t inUseMutex; 	// Mutex used to lock the serial control so that one thread can write a command
									// and read the reply without being interrupted.
	static SerialControl *uniqueInstance;	// Unique instance of this class, to be used by all.
									// If this instance is NULL a new one will be created.
	unsigned char* send(unsigned char buffer[]);
	unsigned char calcCheck1(unsigned char buffer[]);
	unsigned char calcCheck2(unsigned char checksum1);
	void calcChecksums(unsigned char buffer[]);
};

#endif /* SERIALCONTROL_H_ */
