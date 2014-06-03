/*
 * TCP.h
 *
 *  Created on: 31 mei 2014
 *      Author: Roald
 *
 */

// This file is currently obsolete due to the TCP header never being read from the socket.
// Leaving it for now in case another use is found
#ifndef TCP_H_
#define TCP_H_

struct tpc_header
{
	int16_t source_port;
	int16_t dest_port;
	int32_t seq_num;
	int32_t ack_num;
	int data_offset;
	char reserved[3];
	char flags[9];
	int16_t window;
	int16_t checksum;
	int16_t urgent_pointer;
};

#endif /* TCP_H_ */
