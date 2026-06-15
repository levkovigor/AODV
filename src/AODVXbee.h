#ifndef __AODVXBEE_H__
#define __AODVXBEE_H__

#include "StringU8.h"
#include "Arduino.h"
#include "AODVConfig.h"

struct aodvxbee_packet {
	uint8_t  type;
	uint8_t  frameID;
	StringU8 atcommand;
	uint8_t  status;
	StringU8 payload;
	uint64_t destination;
};	

class AODVXbee
{
	public:
	
	AODVXbee();
	
	void begin(HardwareSerial &serial, long baudrate);
	
	
	aodvxbee_packet process();
	
	aodvxbee_packet processBuffer();
	
	void sendResponse(aodvxbee_packet pk);
	
	String byteArrToString(uint8_t *arr, int len);
	
	bool checkCRC();
	
	HardwareSerial* _serial;
	
	
	uint8_t buffer[400];
	
	bool escape = false;
	
	int iterator = 0;
	
	int buf_length = 0;
	
	bool enabled = false;
	
	
	long _baudrate = 115200;
	
};

#endif // __AODVPACKETQUEUEE_H__