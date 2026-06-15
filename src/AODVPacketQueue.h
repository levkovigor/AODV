#ifndef __AODVPACKETQUEUE_H__
#define __AODVPACKETQUEUE_H__

#include "Arduino.h"
#include "LinkedList.h"
#include "AODVConfig.h"
#include "AODVPacket.h"

struct AODVPacketQueueRow {
	uint8_t 		type;
	uint16_t 		seqID;
	uint16_t 		genID;
	uint8_t 		frameID;
	uint8_t  		retries;
	uint8_t  		origRetries;
	uint64_t 		sourceMAC;
	uint64_t 		destinationMAC;
	unsigned long 	timestamp;
	uint8_t  		command;
	StringU8 		payload;
	uint8_t 		hopCount;
	unsigned long 	delay;
};

class AODVPacketQueue
{
	public:
	
	AODVPacketQueue();
	
	void addToQueue(uint8_t type, uint8_t command, uint16_t seqID, uint16_t genID, uint8_t frameID, uint64_t sourceID, uint64_t destinationID, StringU8 payload, uint8_t hopCount = 0, unsigned long delay = 0, uint8_t origRetries = 0);
		
	LinkedList<AODVPacketQueueRow> queue;
	
	int iterator = 0;
	
};

#endif // __AODVPACKETQUEUE_H__