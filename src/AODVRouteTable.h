#ifndef __AODVROUTETABLE_H__
#define __AODVROUTETABLE_H__

#include "Arduino.h"
#include "LinkedList.h"
#include "AODVConfig.h"
#include "AODVPacket.h"

struct AODVRouteTableRow {
	uint16_t 	  seqID;
	uint16_t 	  genID;
	uint8_t  	  frameID;
	uint8_t  	  retries;
	uint8_t  	  type;
	uint8_t  	  command;
	uint64_t 	  neighbourMAC;
	uint64_t 	  destinationMAC;
	uint8_t  	  hopCount;
	float 	 	  rssi;
	unsigned long lifetime;
};

class AODVRouteTable
{
	public:
	
	AODVRouteTable();
	
	uint8_t addToTable(AODVPacket pk, float rssi = 0);
	
	LinkedList<AODVRouteTableRow> table;
	
};

#endif // __AODVROUTETABLE_H__