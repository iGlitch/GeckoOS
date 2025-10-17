#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <ogc/exi.h>
#include <ogc/machine/processor.h>
#include "fst.h"
#include "sd.h"
#include "fakesign.h"
#include "identify.h"
#include "gfx.h"
#include "menu.h"
#include "tools.h"
#include "load.h"
#include "apploader.h"
#include "identify.h"
#include "patchcode.h"
#include "sysmenu.h"
#include "main.h"

#define sdbuffer 0x90080000

u8 gecko_command = 0;
u32 gecko_attached = 0;
u32 gecko_channel;
u8 autobootcheck;
u8 autoboot = 0;
u8 autobootwait = 4;
u8 autoboothbc = 0;
u8 disableRebooter = 0;
u8 buggycIOS = 0;
u8 identifyworks = 1;
u8 loaderhbc = 0;
u8 startupiosloaded = 0;
u8 progmode = 0;

extern void gfx_render_direct();
extern void gfx_init_stars();

//---------------------------------------------------------------------------------
static void power_cb()
//---------------------------------------------------------------------------------
{
	STM_ShutdownToStandby();
}

//---------------------------------------------------------------------------------
static void reset_cb()
//---------------------------------------------------------------------------------
{
	STM_RebootSystem();
}

static __inline__ int __send_command1(s32 chn,u16 *cmd)
{
	s32 ret;

	ret = 0;
	if(EXI_Lock(chn,EXI_DEVICE_0,NULL)) {
		if(!EXI_Select(chn,EXI_DEVICE_0,EXI_SPEED32MHZ)) ret |= 0x01;
		if(!EXI_Imm(chn,cmd,sizeof(u16),EXI_READWRITE,NULL)) ret |= 0x02;
		if(!EXI_Sync(chn)) ret |= 0x04;
		if(!EXI_Deselect(chn)) ret |= 0x08;
		if(!EXI_Unlock(chn)) ret |= 0x10;

		if(ret) ret = 0;
		else ret = 1;
	}
	return ret;
}

static int __usb_recvbyte1(s32 chn,char *ch)
{
	s32 ret;
	u16 val;

	*ch = 0;
	val = 0xA000;
	ret = __send_command1(chn,&val);
	if(ret==1 && !(val&0x0800)) ret = 0;
	else if(ret==1) *ch = (val&0xff);

	return ret;
}



void usb_flushnew(s32 chn)
{
	char tmp;
	int i;

	for(i=0;i<256;i++)	// chip has a 256 byte buffer only
	{
		__usb_recvbyte1(chn,&tmp);
	}
}

void loadStartupIOS()
{
	if (!startupiosloaded)
	{
		//sdio_Shutdown();
		if (IOS_GetVersion() != 36) IOS_ReloadIOS(36);
		detectIOScapabilities();
		
		if (!identifyworks && !isIOSstub(249) && IOS_ReloadIOS(249) >= 0)
			detectIOScapabilities();
		
		if (!identifyworks && IOS_ReloadIOS(35) >= 0)
			detectIOScapabilities();
		
		if (!identifyworks)
		{
			IOS_ReloadIOS(36);
			disableRebooter = 1;
		}
		
		sd_init();
		startupiosloaded = 1;
	}
}

void detectIOScapabilities()
{
	if (!(identifyworks = DIVerify_works()) || identify_SU() < 0 || !DIVerify_works())
	{
		if (identifyworks)
		{
			menu_generatechannellist(TRUE);	// DIVerify seems to be slightly broken, try to workaround the problem
			IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
			buggycIOS = 1;
			identifyworks = (channellist_menu_items > 0);
		}
	}
	if (IOS_GetVersion() == 249 && IOS_GetRevision() > 7 && IOS_GetRevision() < 14)
		disableRebooter = 1;
	else
		disableRebooter = 0;
}

bool isIOSstub(u8 ios_number)
{
	static signed_blob ios_tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	u32 tmd_size;
	tmd *ios_tmd;
	
	if (ios_number == 249)
	{
		ES_GetStoredTMDSize(0x0000000100000000ULL | ios_number, &tmd_size);
		ES_GetStoredTMD(0x0000000100000000ULL | ios_number, ios_tmd_buf, tmd_size);
		ios_tmd = SIGNATURE_PAYLOAD(ios_tmd_buf);
		if (ios_tmd->title_version >= 65280) return TRUE;
	}
	
	return FALSE;
}


//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	if ( (*(u32*)(0xCD8005A0) >> 16 ) == 0xCAFE ) // Wii U
	{
		/* vWii widescreen patch by tueidj */
		write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);
	}
	s32 ret;
	u8 gamestatus = 0x03;
	u8 versionnumber = 0x80;
	u32 geckoidcheck;
	u8 oldconfigbytes[2];
	
	ret = sd_init();
	if(!ret){
		sd_found = 0;
	}
	else {
		sd_found = 1;
	}
	
	if (*((u32 *) 0x80001804) == 0x53545542 && *((u32 *) 0x80001808) == 0x48415858)
		loaderhbc = 1;
	
	*(u32*)0xCD00643C = 0x00000000;	// 32Mhz on Bus
	
	gecko_attached = usb_isgeckoalive(EXI_CHANNEL_1);
	if(gecko_attached){	
		gecko_channel = 1;
		if (EXI_GetID(gecko_channel, 0x80, &geckoidcheck) == 1)
		{
			if (geckoidcheck != 0)
			{
				gecko_attached = FALSE;
				goto slota;
			}
		}
		else
		{
			gecko_attached = FALSE;
			goto slota;
		}
		usb_flushnew(gecko_channel);
		goto slotb;
	}
	
slota:
	
	gecko_attached = usb_isgeckoalive(EXI_CHANNEL_0);
	if(gecko_attached){	
		gecko_channel = 0;
		if (EXI_GetID(gecko_channel, 0x80, &geckoidcheck) == 1)
		{
			if (geckoidcheck != 0)
			{
				gecko_attached = FALSE;
				goto slotb;
			}
		}
		else
		{
			gecko_attached = FALSE;
			goto slotb;
		}
		usb_flushnew(gecko_channel);
	}
	
slotb:
	
	sd_load_config();
	
	tempgameconf = (char *) sdbuffer;
	autobootcheck = 1;
	memset(gameidbuffer, 0, 8);
	app_loadgameconfig(gameidbuffer);
	autobootcheck = 0;
	
	loadStartupIOS();
	iosversion = IOS_GetVersion();
	
	if(config_not_loaded == 0){
		menu_load_config();
		config_not_loaded = 1;
	}
	
	if (autoboothbc || !loaderhbc)
	{
		if (autoboot == 1)
		{
			rebooter_thread();
			menu_number = 12;
			rebooter_thread_state = 0;
		}
	}
	else
		autoboot = 0;
	
	WPAD_Init();
	PAD_Init();
	VIDEO_Init();
	AUDIO_Init (NULL);
	
	SYS_SetPowerCallback (power_cb);
    SYS_SetResetCallback (reset_cb);
	
	if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable()) {
		progmode = 1;
	}

	if (CONF_GetAspectRatio()) {
		widescreen = 1;
	}
	
	gfx_init();
	gfx_load_gfx();
	
	gfx_load_gfx1();
	if (autoboot == 0 || autobootwait > 1)
		gfx_fade_logo();
	
	VIDEO_WaitVSync();
	
	if(!gecko_attached){	
		gecko_channel = 2;
	}

	gfx_init_stars();
	
	while(1)
	{
		if(gecko_attached){
			usb_recvbuffer(gecko_channel,&gecko_command,1);
			switch(gecko_command)
			{
				//case 0x04:	
				//	gecko_readmem();	
				//break;
				
				case 0x14:		// Load DOL
					load_geckoexe(0);
				break;
					
				case 0x24:		// Load DOL
					load_geckoexe(1);
				break;

				case 0x42:
					// Debugger on, pause start off
					config_bytes[7] = 0x01;
					config_bytes[5] = 0x00;
					usb_recvbuffer_safe(gecko_channel,&oldconfigbytes,2);	// Get config
					config_bytes[0] = oldconfigbytes[0];
					switch (oldconfigbytes[1])
					{
					case 0x00:
						config_bytes[1] = 0x00;
						break;
					case 0x01:
						config_bytes[1] = 0x01;
						break;
					case 0x02:
						config_bytes[1] = 0x00;
						break;
					case 0x03:
						config_bytes[1] = 0x01;
						break;
					case 0x04:
						config_bytes[1] = 0x03;
						break;
					case 0x05:
						config_bytes[1] = 0x03;
						break;
					case 0x06:
						config_bytes[1] = 0x02;
						break;
					case 0x07:
						config_bytes[1] = 0x02;
						break;
					}
					menu_number = 8;
					apploader_thread();
					gecko_command = 0;
				break;
					
				case 0x43:
					// Debugger on, pause start on
					config_bytes[7] = 0x01;
					config_bytes[5] = 0x01;
					usb_recvbuffer_safe(gecko_channel,&oldconfigbytes,2);	// Get config
					config_bytes[0] = oldconfigbytes[0];
					switch (oldconfigbytes[1])
					{
						case 0x00:
							config_bytes[1] = 0x00;
							break;
						case 0x01:
							config_bytes[1] = 0x01;
							break;
						case 0x02:
							config_bytes[1] = 0x00;
							break;
						case 0x03:
							config_bytes[1] = 0x01;
							break;
						case 0x04:
							config_bytes[1] = 0x03;
							break;
						case 0x05:
							config_bytes[1] = 0x03;
							break;
						case 0x06:
							config_bytes[1] = 0x02;
							break;
						case 0x07:
							config_bytes[1] = 0x02;
							break;
					}
					menu_number = 8;
					apploader_thread();
					gecko_command = 0;
				break;

				case 0x50:		
					usb_sendbuffer_safe(gecko_channel,&gamestatus,1);
				break;

				case 0x99:		
					usb_sendbuffer_safe(gecko_channel,&versionnumber,1); 
				break;
			}

		}
		
		if(error_sd || confirm_sd || error_video){
			sleep(1);
			error_sd = 0;
			confirm_sd = 0;
			error_video = 0;
		}

		if(loadedpng && !alphapng){	// if custom backdrop draw bubbles after image
			gfx_draw_image(0, 0, 640, 480, tex_logo1, 0, 1, 1,0xff);
			gfx_draw_stars();
		}
		else{
			gfx_draw_stars(); // else if own back drop do in this order due to border clip
			gfx_draw_image(0, 0, 640, 480, tex_logo1, 0, 1, 1,0xff);
		};
		
		menu_draw();
		gfx_render_direct();
	}

	return 0;
}
