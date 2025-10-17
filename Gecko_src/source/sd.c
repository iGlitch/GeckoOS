#include <gccore.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/dir.h>
#include <ogc/exi.h>

#include "sd.h"
#include "main.h"

u8 config_bytes[no_config_bytes] ATTRIBUTE_ALIGN(32);

void usb_flushnew(s32 chn);
u32 sd_found;

//---------------------------------------------------------------------------------
u32 sd_shutdown()
//---------------------------------------------------------------------------------
{
	return 0;
}

//---------------------------------------------------------------------------------
void sd_refresh()
//---------------------------------------------------------------------------------
{
	s32 ret = 0;
	u32 geckoidcheck;
	
	sd_shutdown();
	
	ret = sd_init();
	if(ret){
		sd_found = 1;
	}
	else {
		sd_found = 0;
	}
	sd_load_config();

	// Check USB Gecko
	gecko_attached = usb_isgeckoalive(EXI_CHANNEL_1);
	if(gecko_attached){	
		gecko_channel = 1;
		if (EXI_GetID(gecko_channel, 0x80, &geckoidcheck) == 1)
		{
			if (geckoidcheck != 0)
			{
				gecko_attached = FALSE;
				goto inslota;
			}
		}
		else
		{
			gecko_attached = FALSE;
			goto inslota;
		}
		usb_flushnew(gecko_channel);
		goto inslotb;
	}
	
inslota:
	
	gecko_attached = usb_isgeckoalive(EXI_CHANNEL_0);
	if(gecko_attached){	
		gecko_channel = 0;
		if (EXI_GetID(gecko_channel, 0x80, &geckoidcheck) == 1)
		{
			if (geckoidcheck != 0)
			{
				gecko_attached = FALSE;
				goto inslotb;
			}
		}
		else
		{
			gecko_attached = FALSE;
			goto inslotb;
		}
		usb_flushnew(gecko_channel);
	}
	
inslotb:

	VIDEO_WaitVSync();

	if(!gecko_attached){	
		gecko_channel = 2;
	}
}

//---------------------------------------------------------------------------------
u32 sd_init()
//---------------------------------------------------------------------------------
{
	return fatInitDefault();
}


//---------------------------------------------------------------------------------
void sd_check_mkdir(char *path)
//---------------------------------------------------------------------------------
{
/*	DIR_ITER *pdir;

	pdir = opendir(path);
	if(pdir == NULL){
		mkdir(path, 0);
		return;
	}

	closedir(pdir);
*/}

//---------------------------------------------------------------------------------
u32 sd_load_config()
//---------------------------------------------------------------------------------
{
	char filepath[MAX_FILEPATH_LEN];
	int ret = 0;
	FILE *fp = NULL;
	
	// Set defaults, will get ovewritten if config.dat exists
	config_bytes[0] = 0xCD;
	config_bytes[1] = 0x00;
	config_bytes[2] = 0x08;
	config_bytes[3] = 0x00;
	config_bytes[5] = 0x00;	// no paused start
	config_bytes[6] = 0x00; // gecko slot b	
	config_bytes[8] = 0x00;
	config_bytes[9] = 0x00;
	config_bytes[10] = 0x00;
	config_bytes[11] = 0x00;
	if (gecko_attached)
	{
		config_bytes[4] = 0x00;
		config_bytes[7] = 0x01;
	}
	else
	{
		config_bytes[4] = 0x01;
		config_bytes[7] = 0x00;
	}
	
	if (sd_found == 0)
		return 0;
	
	sprintf (filepath, "sd:/data/gecko/config.dat");
	fp = fopen(filepath, "rb");
	if (!fp){
		sprintf (filepath, "sd:/config.dat");
		fp = fopen(filepath, "rb");
		if (!fp){
			return 0;
		}
	};

	ret = fread(config_bytes, 1, no_config_bytes, fp);
	DCFlushRange(config_bytes, no_config_bytes);

	if(ret != no_config_bytes){
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

//---------------------------------------------------------------------------------
u32 sd_save_config()
//---------------------------------------------------------------------------------
{
	char filepath[MAX_FILEPATH_LEN];
	int ret;
	
	sprintf (filepath, "sd:/config.dat");
	FILE *fp = fopen(filepath, "wb");
	if (!fp){
		return 0;
	};

	ret = fwrite(config_bytes, 1, no_config_bytes, fp);
	DCFlushRange(config_bytes, no_config_bytes);

	if(ret != no_config_bytes){
		fflush(fp);
		fclose(fp);
		return 0;
	}

	fflush(fp);
	fclose(fp);

	return 1;
}
