/*
 * I2CControl.h
 *
 *  Created on: 16 jul. 2014
 *      Author: Asus
 */

#ifndef I2CCONTROL_H_
#define I2CCONTROL_H_

#define TWOWIREDEVICE "/dev/i2c-1"
#define DEVICE 0x70

namespace std {

class I2CControl {
public:
	I2CControl();
	virtual ~I2CControl();

	void test_read();
private:
	int filepointer;
};

} /* namespace std */
#endif /* I2CCONTROL_H_ */
