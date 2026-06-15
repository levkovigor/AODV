#ifndef __AODV_H__
#define __AODV_H__

#include "StringU8.h"

#include "Arduino.h"
#include "AODVConfig.h"
#include "AODVPacket.h"
#include "AODVRouteTable.h"
#include "AODVPacketQueue.h"
#include "AODVXbee.h"
#include <EEPROM.h>

struct AODVSettings {
	uint8_t magic;
	uint8_t routingMode;
	char nodeName[32];
	uint16_t networkID;
	uint8_t preambleID;
	uint8_t txPowerLevel;
	uint8_t baudrateIndex;
	uint8_t parity;
	uint8_t stopBits;
	uint8_t broadcastMultiTransmits;
	uint8_t unicastMacRetries;
	uint8_t broadcastHops;
	uint8_t networkHops;
	uint8_t networkDelaySlots;
	uint8_t meshUnicastRetries;
	uint16_t nodeDiscoverTimeout;
};

class AODV
{
	public:
	
	//Settings	
	uint64_t MAC_ID;
	
	String nodeName;
	
	uint8_t routingMode = 0;
	
	uint16_t networkID = 0x7FFF;
	uint8_t preambleID = 0x00;
	uint8_t txPowerLevel = 0x04;
	uint8_t baudrateIndex = 0x07;
	uint8_t parity = 0x00;
	uint8_t stopBits = 0x00;
	uint8_t broadcastMultiTransmits = 0x03;
	uint8_t unicastMacRetries = 0x0A;
	uint8_t broadcastHops = 0x00;
	uint8_t networkHops = 0x07;
	uint8_t networkDelaySlots = 0x03;
	uint8_t meshUnicastRetries = 0x01;
	uint16_t nodeDiscoverTimeout = 0x0082;
	
	AODV(uint64_t id = 0);
	
	String macToStr(uint64_t id);
	
	uint64_t strToMac(String mac);
	
	void setMAC(uint64_t id);
	
	void setNodeName(String name);

	void loadSettings();
	void saveSettings();
	void restoreDefaults();
	long indexToBaudrate(uint8_t index);
	
	uint16_t sequence_ID;
	
	AODVPacket transmit(int rowID);
	
	AODVPacket receive(uint8_t* raw, int length, float rssi = 0);
	
	uint64_t getReceiverMac(uint64_t destID);
	
	AODVPacket receivePacket(AODVPacket pk, float rssi = 0);
	
	void printTable();
	
	void printPacketData(AODVPacket pk);
	
	AODVRouteTable table;
	
	AODVPacketQueue queue;
	
	AODVXbee xbee;
	
	void xbeeProcess();
	
	int queueProcess();
	
	bool checkQueueReply(AODVPacket pk, float rssi);
	
	float last_rssi = 0;
	
	aodvxbee_packet xbeeATCommandResponseGet(aodvxbee_packet pk);
	
	aodvxbee_packet xbeeATCommandResponseSet(aodvxbee_packet pk);
	
	//AODVPacket packet;
	
	aodvxbee_packet generateZBRXResponsePacket(AODVPacket pk, float rssi);

	aodvxbee_packet generateMACReplyPacket(AODVPacket pk, float rssi);
	
	aodvxbee_packet generateRemoteATResponse(AODVPacket pk, float rssi);
	
	aodvxbee_packet generateZBTXStatusResponse(AODVPacket pk, float rssi);
	
};

#endif // __AODV_H__