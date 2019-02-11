#include <gctypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gccore.h>	
#include <malloc.h>

#include "tools.h"
#include "main.h"
#include "geckomem.h"

#define packetsize 63488

int gecko_readmem()
{
	u8 startaddress[4];
	u8 endaddress[5];
	u8 ack_packet = 0xAA;
	u8 pcresponse;
	static u8 packetbuffer[packetsize] ATTRIBUTE_ALIGN(32);
	u32 memstart;
	u32 memend;
	u32 memrange;
	u32 fullpackets;
	u32 currentpacket;
	u32 lastpacketbytes;

	// ACK
	usb_sendbuffer_safe(gecko_channel,&ack_packet,1);
	usb_recvbuffer_safe(gecko_channel,&startaddress,4);		
	usb_recvbuffer_safe(gecko_channel,&endaddress,4);		
	
	memstart = be32(startaddress);	
	memend = be32(endaddress);	
	memrange = memend - memstart;

	fullpackets = memrange / packetsize;						
	lastpacketbytes = memrange % packetsize;					

	// Full Packet Size
	for (currentpacket = 0;currentpacket < fullpackets;)
	{	
		memcpy(packetbuffer, (char*)memstart, packetsize);
		usb_sendbuffer(gecko_channel,&packetbuffer, packetsize);
		usb_recvbuffer_safe(gecko_channel,&pcresponse,1);	
		if(pcresponse != 0xAA){								
		//	printf("failed to read packet\n");
			return 0;
		}
			memstart += packetsize;
			currentpacket++;	
	}

	// Remainder
	if(lastpacketbytes > 0) {
		memcpy(packetbuffer, (char*)memstart, lastpacketbytes);
		usb_sendbuffer_safe(gecko_channel,&packetbuffer, lastpacketbytes);
		usb_recvbuffer_safe(gecko_channel,&pcresponse,1);	
	}
	
//	printf("memory dump complete\n");
	return 1;
}

int gecko_readrange(u32 memstart, u32 memend)
{

	char pcresponse;
	static char packetbuffer[packetsize] ATTRIBUTE_ALIGN(32);
	u32 memrange;
	u32 fullpackets;
	u32 currentpacket;
	u32 lastpacketbytes;

	memrange = memend - memstart;
	
	fullpackets = memrange / packetsize;						
	lastpacketbytes = memrange % packetsize;		

	// Full Packet Size
	for (currentpacket = 0;currentpacket < fullpackets;)
	{	
		memcpy(packetbuffer, (char*)memstart, packetsize);
		usb_sendbuffer_safe(gecko_channel,&packetbuffer, packetsize);
		usb_recvbuffer_safe(gecko_channel,&pcresponse,1);	
		if(pcresponse != 0xAA){								
			return 0;
		}
			memstart += packetsize;
			currentpacket++;	
	}
	// Remainder
	if(lastpacketbytes > 0) {
		memcpy(packetbuffer, (char*)memstart, lastpacketbytes);
		usb_sendbuffer_safe(gecko_channel,&packetbuffer, lastpacketbytes);
		
	}
	return 1;
}
