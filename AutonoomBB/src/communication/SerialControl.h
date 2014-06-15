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
#define UART_BUFFER_SIZE 256
#define TRUE 1
#define FALSE 0

class SerialControl {
public:
	virtual ~SerialControl();
	static SerialControl* getInstance();

	void restoreDefault();
	void setup();

	unsigned char* send_calc_checksum(unsigned char buffer[]);
	unsigned char* send(unsigned char arguments[], std::string command, int argument_length);

	int find_command_id(std::string command);
private:
	SerialControl();
	unsigned char* send_ijog(unsigned char arguments[], int argument_length);
	unsigned char* send_sjog(unsigned char arguments[], int argument_length);
	unsigned char* send_command(int command_id, unsigned char arguments[], int argument_length);

	unsigned char* read_serial(int bytes_expected);

	void print_buffer(unsigned char buffer[], int lenght);
	void calc_checksums(unsigned char buffer[]);
	void flush_serial_port();

	int fileDescriptor;		// Pointer to the device file
	pthread_t serialThread;	// Thread controlling serial trafic
	pthread_mutex_t inUseMutex; 	// Mutex used to lock the serial control so that one thread can write a command
									// and read the reply without being interrupted.
	static SerialControl *uniqueInstance;	// Unique instance of this class, to be used by all.
									// If this instance is NULL a new one will be created.
};

#endif /* SERIALCONTROL_H_ */
