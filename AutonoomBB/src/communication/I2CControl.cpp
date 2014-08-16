/*
 * I2CControl.cpp
 *
 *  Created on: 16 jul. 2014
 *      Author: Asus
 */

#include "I2CControl.h"

#include "linux/i2c-dev.h"
#include "linux/i2c.h"
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

using namespace std;

I2CControl::I2CControl()
	:filepointer(0)
{

}

I2CControl::~I2CControl()
{

}

void I2CControl::test_read()
{
	unsigned char transmit_buffer[10];
	unsigned char receive_buffer[10];
	int bytes;
	int status;

	memset(transmit_buffer, 0, 10);
	memset(receive_buffer, 0, 10);

	filepointer = open(TWOWIREDEVICE, O_RDWR);

	if(filepointer < 0)
	{
		cout << "Could not open i2c bus" << endl;
		return;
	}

	status = ioctl(filepointer, I2C_SLAVE, DEVICE);
	if(status < 0)
	{
		cout << "Could not open bus or talk to slave" << endl;
		return;
	}

	transmit_buffer[0] = 0x00;

	if((bytes = write(filepointer, transmit_buffer, 1)) != 1)
	{
		cout << "Failed to write to bus " << errno << endl;
		//return;
	}

	if((bytes = read(filepointer, receive_buffer, 1)) != 1)
	{
		cout << "No reply was received" << bytes << endl;
		return;
	} else cout << "Received reply: " << receive_buffer << endl;
}
