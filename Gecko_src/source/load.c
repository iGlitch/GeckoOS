#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gccore.h>
#include <ogcsys.h>

#include "load.h"
#include "tools.h"
#include "relos.h"
#include "reloself.h"
#include "apploader.h"
#include "main.h"
//#include "dol_loader.h"

#define reloc_code_location 0x811f8000
#define exe_file_location 0x90080000
#define packetsize 3968

typedef void (*entrypoint) (void);

//---------------------------------------------------------------------------------
void load_geckoexe(u32 mode) // 0 for DOL, 1 for ELF, 2 for DOL debug, 3 for ELF debug
//---------------------------------------------------------------------------------
{

	u8 exesize[4];
	u32 exe_filesize;
	u32 fullpackets = 0;
	u32 currentpacket = 0;
	u32 last_bytes = 0;
	static u8 packetbuff[packetsize] ATTRIBUTE_ALIGN(32);
	u8 *exe_buff = (u8*)exe_file_location;
	u8 ack_packet = 0xAA;
	
	WPAD_Shutdown();
	//sdio_Shutdown();

	usb_sendbuffer_safe(gecko_channel,&ack_packet,1);

	usb_recvbuffer_safe(gecko_channel,&exesize, 4);
	exe_filesize = be32(exesize);

	memset((u8*)exe_file_location, 0, exe_filesize);			
	
	fullpackets = exe_filesize / packetsize;				
    last_bytes  = exe_filesize % packetsize;		
//	doprogress(0,fullpackets); // Init and draw progress box

	for (currentpacket=0;currentpacket < fullpackets;)
	{
		memset(packetbuff, 0, packetsize);
		usb_recvbuffer_safe(gecko_channel,&packetbuff, packetsize);
		memcpy(exe_buff, packetbuff, packetsize);
		exe_buff += packetsize;
		currentpacket++;		
//		doprogress(1,fullpackets); // Step
		usb_sendbuffer_safe(gecko_channel,&ack_packet,1);
	}
//	doprogress(2,fullpackets); // Padding
	// Dump remainder and flush
	usb_recvbuffer_safe(gecko_channel,&packetbuff, last_bytes);
	memcpy(exe_buff, &packetbuff, last_bytes);
	DCFlushRange((u8*)exe_file_location, exe_filesize);
	// Copy stub
	
	
	switch(mode)
	{
		case 0:
			//memcpy((void*)0x80001800, relostub, relostub_size);
			//DCFlushRange((void*)0x80001800, relostub_size);
			codes_state = 0;
			if (config_bytes[7] == 0x00)
				codelist = (u8 *) 0x800022A8;
			else
				codelist = (u8 *) 0x800028B8;
			codelistend = (u8 *) 0x80003000;
			//determine_libogc_hook(exe_file_location, exe_filesize - 4);
			config_bytes[2] = 0x01;
			load_handler();
			memcpy((u8*)reloc_code_location, relos, relos_size); // DOL
			DCFlushRange((u8*)reloc_code_location, relos_size);
			break;
			
		case 1:
			//memcpy((void*)0x80001800, relostub, relostub_size);
			//DCFlushRange((void*)0x80001800, relostub_size);
			memcpy((u8*)reloc_code_location, reloself, reloself_size); //ELF
			DCFlushRange((u8*)reloc_code_location, reloself_size);
			break;
	}
	
	void (*exe_entry)(void*) = reloc_code_location;
	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	exe_entry((void*)exe_file_location);	// disables IRQ and relocates from very high
}
