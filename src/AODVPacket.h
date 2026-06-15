#ifndef __AODVPACKET_H__
#define __AODVPACKET_H__

#include "StringU8.h"
#include "Arduino.h"
#include "AODVConfig.h"

struct aodv_packet {
	uint8_t	 type;
	uint64_t transmitterMAC;
	uint64_t receiverMAC;
	uint16_t seqID;
	uint16_t genID;
	uint8_t  frameID;
	uint8_t  retries;
	uint64_t sourceMAC;
	uint64_t destinationMAC;
	uint8_t  hopCount;
	uint16_t lifetime;
	uint8_t	 command;
	StringU8   payload;
};

class AODVPacket
{
	public:
		
	AODVPacket(uint8_t type = 0, uint64_t transmitterMAC = 0, uint64_t receiverMAC = 0, uint16_t seqID = 0, uint16_t genID = 0, uint64_t sourceMAC = 0, uint8_t frameID = 0, uint8_t retries = 0, uint64_t destinationMAC = 0, uint8_t hopCount = 0, uint16_t lifetime = 0, uint8_t command = 0, StringU8 payload = "");
	
	uint8_t* generateRawData();
	
	int rawDataLength();
	
	void parseRawData(uint8_t* raw, int length);
	
	aodv_packet packet;
	
	uint8_t rawData[400];
	
};

#endif // __AODVPACKET_H__