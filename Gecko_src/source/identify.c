#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogc/es.h>
#include <ogcsys.h>
#include <sys/unistd.h>
#include <fat.h>
#include "sdcard/wiisd_io.h"
#include <wiiuse/wpad.h>
#include "dvd_broadway.h"
#include "apploader.h"
#include "patchcode.h"
#include "identify.h"
#include "menu_tik_bin.h"
#include "menu_tmd_bin.h"
#include "menu_certs_bin.h"
#include "empty_tmd_bin.h"
#include "empty_tik_bin.h"
#include "sysmenu.h"
#include "multidol.h"
#include "tools.h"
#include "certs_dat.h"
#include "fakesign.h"
#include "main.h"

#define sdbuffer 0x90080000

int rebooter_running = 0;
int rebooter_thread_state = 0;
lwp_t rebooterthread;
u32 *bootentrypoint;
int channel_running = 0;
int channel_thread_state = 0;
lwp_t channelthread;
u64 channeltoload = 0x0;
u8 channelios;
u8 channelidentified = 0;
u32 bootid;
u16 bootindex;
//static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";
static u8 tmdbuf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(0x20);
static u8 tikbuf[STD_SIGNED_TIK_SIZE] ATTRIBUTE_ALIGN(0x20);
static u8 su_tmd[0x208] ATTRIBUTE_ALIGN(32);
static u8 su_tik[STD_SIGNED_TIK_SIZE] ATTRIBUTE_ALIGN(32);
int su_id_filled = 0;

typedef struct _dolheader
{
	u32 text_pos[7];
	u32 data_pos[11];
	u32 text_start[7];
	u32 data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;

u32 DIVerify_works()
{
	s32 ret;
	
	ret = ES_Identify((signed_blob *)certs_dat, certs_dat_size, (signed_blob *)empty_tmd_bin, empty_tmd_bin_size, (signed_blob *)empty_tik_bin, empty_tik_bin_size, NULL);
	if (ret == -2011 || ret >= 0) return 1;
	return 0;
}

s32 Identify(const u8 *certs, u32 certs_size, const u8 *idtmd, u32 idtmd_size, const u8 *idticket, u32 idticket_size) {
	s32 ret;
	u32 keyid = 0;
	ret = ES_Identify((signed_blob*)certs, certs_size, (signed_blob*)idtmd, idtmd_size, (signed_blob*)idticket, idticket_size, &keyid);
	if (ret < 0){
		switch(ret){
			case ES_EINVAL:
				printf("Error! ES_Identify (ret = %d;) Data invalid!\n", ret);
				break;
			case ES_EALIGN:
				printf("Error! ES_Identify (ret = %d;) Data not aligned!\n", ret);
				break;
			case ES_ENOTINIT:
				printf("Error! ES_Identify (ret = %d;) ES not initialized!\n", ret);
				break;
			case ES_ENOMEM:
				printf("Error! ES_Identify (ret = %d;) No memory!\n", ret);
				break;
			default:
				printf("Error! ES_Identify (ret = %d)\n", ret);
				break;
		}
	}
	else
		printf("OK!\n");
	return ret;
}

void identify_makeSU()
{
	signed_blob *s_tmd, *s_tik;
	tmd *p_tmd;
	tik *p_tik;
	
	memset(su_tmd, 0, sizeof su_tmd);
	memset(su_tik, 0, sizeof su_tik);
	s_tmd = (signed_blob*)&su_tmd[0];
	s_tik = (signed_blob*)&su_tik[0];
	*s_tmd = *s_tik = 0x10001;
	p_tmd = (tmd*)SIGNATURE_PAYLOAD(s_tmd);
	p_tik = (tik*)SIGNATURE_PAYLOAD(s_tik);
	
	
	strcpy(p_tmd->issuer, "Root-CA00000001-CP00000004");
	p_tmd->title_id = TITLE_ID(1,2);
	
	p_tmd->num_contents = 1;
	
	forge_tmd(s_tmd);
	
	strcpy(p_tik->issuer, "Root-CA00000001-XS00000003");
	p_tik->ticketid = 0x000038A45236EE5FLL;
	p_tik->titleid = TITLE_ID(1,2);
	
	memset(p_tik->cidx_mask, 0xFF, 0x20);
	forge_tik(s_tik);
	
	su_id_filled = 1;
	
}


s32 identify_SU()
{
	if (!su_id_filled)
		identify_makeSU();
	
	printf("\nIdentifying as SU...");
	fflush(stdout);
	return Identify(certs_dat, certs_dat_size, su_tmd, sizeof su_tmd, su_tik, sizeof su_tik);
}

/*
s32 sys_getcerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[0x280] ATTRIBUTE_ALIGN(32);
	s32 fd, ret;

	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	ret = IOS_Read(fd, certificates, sizeof(certificates));

	IOS_Close(fd);

	if (ret > 0) {
		*certs = certificates;
		*len   = sizeof(certificates);
	}

	return ret;
}
*/

void codehandler_rebooter()
{
	s32 ret;
	
	if (autoboot == 1 && autobootwait > 0)
		sleep(autobootwait);
	autobootwait = 0;
	
	tempcodelist = (u8 *) sdbuffer;
	codelistsize = 0;
	tempgameconf = (char *) sdbuffer;
	gameconfsize = 0;
	codes_state = 0;
	applyfwritepatch = 0;
	
	if (config_bytes[7] == 0x00)
		codelist = (u8 *) 0x800022A8;
	else
		codelist = (u8 *) 0x800028B8;
	codelistend = (u8 *) 0x80003000;
	
	ret = DVDLowInit();
	dvddone = 0;
	ret = DVDLowReset(__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	
	if (config_bytes[2] != 0x00)
	{
		memset(gameidbuffer, 0, 8);
		
		if (config_bytes[2] == 0x08 || config_bytes[4] == 0x01)
		{
			// Clear the Game ID
			memset((char*)0x80000000, 0, 6);
			
			// Get the ID
			dvddone = 0;
			ret = DVDLowReadID(g_diskID,__dvd_readidcb);
			while(ret>=0 && dvddone==0);
			
			memcpy(gameidbuffer, (char*)0x80000000, 6);
			
			app_loadgameconfig(gameidbuffer);
			
			if (config_bytes[7] == 0x00)
				codelist = (u8 *) 0x800022A8;
			else
				codelist = (u8 *) 0x800028E0;
			codelistend = (u8 *) 0x80003000;
			
			if(config_bytes[4] == 0x01 && (gameidbuffer[1] != 0 || gameidbuffer[2] != 0))
				app_copycodes(gameidbuffer);
			else if (gameidbuffer[1] == 0 && gameidbuffer[2] == 0)
				codes_state = 5;
			else
				codes_state = 6;
		}
		
		if (config_bytes[7] == 0x00)
			codelist = (u8 *) 0x800022A8;
		else
			codelist = (u8 *) 0x800028E0;
		codelistend = (u8 *) 0x80003000;
		
		load_handler();
		memset(codelist,0xC,4);
	}
	
	WPAD_Shutdown();
	//sdio_Shutdown();
	
	ret = Menu_identify();
	if(!ret)
	{
		rebooter_thread_state |= 1;
		codes_state = 0;
	}
	if (autoboot == 0)
		sleep(1);
	rebooter_thread_state |= 2;
}

void codehandler_channel()
{
	s32 ret;
	/*u32 count;
	static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	static u64 title_list[256] ATTRIBUTE_ALIGN(32);
	signed_blob *s_tmd;
	u32 tmd_size;
	u32 i;*/
	
	tempcodelist = (u8 *) sdbuffer;
	codelistsize = 0;
	tempgameconf = (char *) sdbuffer;
	gameconfsize = 0;
	codes_state = 0;
	
	memset(gameidbuffer, 0, 8);
	gameidbuffer[0] = (channeltoload & 0xff000000) >> 24;
	gameidbuffer[1] = (channeltoload & 0x00ff0000) >> 16;
	gameidbuffer[2] = (channeltoload & 0x0000ff00) >> 8;
	gameidbuffer[3] = channeltoload & 0x000000ff;
	gameidbuffer[4] = 0;
	
	if (config_bytes[7] == 0x00)
		codelist = (u8 *) 0x800022A8;
	else
		codelist = (u8 *) 0x800028E0;
	codelistend = (u8 *) 0x80003000;
	
	if (config_bytes[2] != 0x00)
	{
		if(config_bytes[3] == 0x01){		
			app_dopatch(gameidbuffer);
		}
		
		app_loadgameconfig(gameidbuffer);
		
		if(config_bytes[4] == 0x01 && (gameidbuffer[1] != 0 || gameidbuffer[2] != 0))
			app_copycodes(gameidbuffer);
		else if (gameidbuffer[1] == 0 && gameidbuffer[2] == 0)
			codes_state = 5;
		else
			codes_state = 6;
	}
	
	// Check that channel exists
	/*ret = ES_GetNumTitles(&count);
	if (ret) {
		channel_thread_state |= 1;
		codes_state = 0;
	}
	else
	{
		ret = ES_GetTitles(title_list, count);
		if (ret) {
			channel_thread_state |= 1;
			codes_state = 0;
		}
		else
		{
			for (i=0; i < count; i++) {
				ret = ES_GetStoredTMDSize(title_list[i], &tmd_size);
				s_tmd = (signed_blob *)tmd_buf;
				ret = ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);
				if (((tmd *)SIGNATURE_PAYLOAD(s_tmd))->title_id == channeltoload)
				{
					channelios = ((tmd *)SIGNATURE_PAYLOAD(s_tmd))->sys_version;
					goto foundtitle;
				}
			}
			channel_thread_state |= 1;
			codes_state = 0;
		}
	}
	
	foundtitle:*/
	
	WPAD_Shutdown();
	//sdio_Shutdown();
	
	// Check that identify works
	ret = Channel_identify();
	if(!ret)
	{
		channel_thread_state |= 1;
		codes_state = 0;
		WPAD_Init();
		while(1);
	}
	
	sleep(1);
	channel_thread_state |= 2;
}

void boot_menu()
{
	s32 ret;
	
	if (buggycIOS) IOS_ReloadIOS(IOS_GetVersion());
	identify_SU();
	
	ret = ISFS_Load_dol(bootid | 0x10000000, 0x0000000100000002ULL);
	if(!ret){
		ret = Menu_identify();
		if(!ret)
			exitme();
		
		ret = ES_Load_dol(bootindex);
		if(!ret)
			exitme();
	}
	else
	{
		ret = Menu_identify();
		if(!ret)
			exitme();
	}
	
	if (config_bytes[2] != 0x00) patchmenu((void*)0x8132ff80, 0x380000, 6);
	
	if (IOS_GetVersion() == 249) patchmenu((void*)0x8132ff80, 0x380000, 7);
	
	// Patches
	if(config_bytes[8] == 0x01){
		patchmenu((void*)0x8132ff80, 0x380000, 0);	// menu size
		return;
	}
	// Region Free
	if(config_bytes[9] == 0x01){
		patchmenu((void*)0x8132ff80, 0x380000, 2);	
	}
	// Health check
	if(config_bytes[11] == 0x01){
		patchmenu((void*)0x8132ff80, 0x380000, 3);
	}
	// No copy bit remove
	if(config_bytes[10] == 0x01){
		patchmenu((void*)0x8132ff80, 0x380000, 4);
	}
	
	if (config_bytes[2] != 0x00) patchmenu((void*)0x8132ff80, 0x380000, 1);
	
	// move dvd channel
	patchmenu((void*)0x8132ff80, 0x380000, 5);
	
	// fwrite Patch
	if (applyfwritepatch){
		patchdebug((void*)0x8132ff80, 0x380000);
	}
	
	DCFlushRange((void*)0x80000000, 0x3f00);
}

void boot_channel()
{
	s32 ret;
	
	/*identify_SU();
	
	ret = ISFS_Load_dol(bootid, channeltoload);
	if(!ret){
		exitme();
	}*/
	
	
	ret = ES_Load_dol(bootindex);
	if(!ret)
		exitme();
	
	*(u16*)0x80003140 = channelios;
	*(u16*)0x80003142 = 0xFFFF;
	
	*(u16*)0x80003188 = channelios;
	*(u16*)0x8000318A = 0xFFFF;
	
	if (config_bytes[2] != 0x00)
	{
		load_handler();
		memcpy((void *)0x80001800, gameidbuffer, 8);
		DCFlushRange((void *)0x80001800, 8);
		app_pokevalues();
		
		if(config_bytes[3] == 0x01){		
			app_apply_patch();
		}
		
		dochannelhooks((void*)0x8132ff80, 0x380000);
	}
	
	DCFlushRange((void*)0x80000000, 0x3f00);
}

void rebooter_thread_close()
{
	if(rebooter_running){
		LWP_SuspendThread(rebooterthread);
		rebooter_running = 0;
	}	
}

void rebooter_thread()
{
	rebooter_thread_state = 0;
	if(!rebooter_running){
		LWP_CreateThread(&rebooterthread, codehandler_rebooter, NULL, NULL, 0, 2);
		rebooter_running = 1;
	}
}

void channel_thread_close()
{
	if(channel_running){
		LWP_SuspendThread(channelthread);
		channel_running = 0;
	}	
}

void channel_thread()
{
	channel_thread_state = 0;
	if(!channel_running){
		LWP_CreateThread(&channelthread, codehandler_channel, NULL, NULL, 0, 2);
		channel_running = 1;
	}
}

s32 Menu_identify()
{
	u64 titleID;
	u32 tmdSize;
	signed_blob *ptmd;
	s32 res;
	signed_blob *p_certs = NULL;
	u32 certs_len;
	u32 i;
	
	// Get certs
	//	res = sys_getcerts(&p_certs, &certs_len);
	//	if (res < 0){
	//		return 0;
	//	}
	
	if (buggycIOS) IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
	
	//Identify as menu.
	res = ES_Identify((signed_blob*)menu_certs_bin, menu_certs_bin_size, (signed_blob*)menu_tmd_bin, menu_tmd_bin_size, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	//res = ES_Identify(p_certs, certs_len, (signed_blob*)menu_tmd_bin, menu_tmd_bin_size, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	if(res < 0){
		return 0;
	}
	
	//Get current title ID.
	ES_GetTitleID(&titleID);
	if(titleID != 0x0000000100000002ULL){
		return 0;
	}
	
	//Get tmd size.
	res = ES_GetStoredTMDSize(titleID, &tmdSize);
	if(res < 0){
		return 0;
	}
	
	//Get tmd.
	ptmd = memalign(32, tmdSize);
	res = ES_GetStoredTMD(titleID, ptmd, tmdSize);
	if(res < 0){
		return 0;
	}
	
	if (buggycIOS) IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
	
	//Identify as menu.
	res = ES_Identify((signed_blob*)menu_certs_bin, menu_certs_bin_size, ptmd, tmdSize, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	//res = ES_Identify(p_certs, certs_len, (signed_blob*)menu_tmd_bin, menu_tmd_bin_size, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	if(res < 0){
		return 0;
	}
	
	ES_GetTitleID(&titleID);
	bootindex = ((tmd *)SIGNATURE_PAYLOAD(ptmd))->boot_index;
	for (i = 0; i < ((tmd *)SIGNATURE_PAYLOAD(ptmd))->num_contents; i++)
	{
		if (((tmd *)SIGNATURE_PAYLOAD(ptmd))->contents[i].index == bootindex)
			bootid = ((tmd *)SIGNATURE_PAYLOAD(ptmd))->contents[i].cid;
	}
	return 1;
}

s32 Channel_identify()
{
	u64 titleID;
	u32 tmdSize = 0, tikSize = 0;
	signed_blob *ptmd;
	s32 res, ret;
	signed_blob *p_certs = NULL;
	u32 certs_len;
	u32 i;
	char filename[256];
	FILE *fp;
	
	if (!channelidentified)
	{
		if (identify_SU() < 0)
			return 0;
		
		if (ISFS_Initialize())
			return 0;
		
		// Get certs
		//	res = sys_getcerts(&p_certs, &certs_len);
		//	if (res < 0){
		//		return 0;
		//	}
		
		snprintf(filename, sizeof(filename), "/title/%08x/%08x/content/title.tmd", TITLE_UPPER(channeltoload), TITLE_LOWER(channeltoload));
		
		ret = ISFS_Open(filename, ISFS_OPEN_READ);
		if(ret < 0){
			ISFS_Deinitialize();
			return 0;
		}
		
		tmdSize = ISFS_Read(ret, (void *) tmdbuf, sizeof(tmdbuf));
		
		ISFS_Close(ret);
		
		snprintf(filename, sizeof(filename), "/ticket/%08x/%08x.tik", TITLE_UPPER(channeltoload), TITLE_LOWER(channeltoload));
		
		ret = ISFS_Open(filename, ISFS_OPEN_READ);
		if(ret < 0){
			ISFS_Deinitialize();
			return 0;
		}
		
		tikSize = ISFS_Read(ret, (void *) tikbuf, sizeof(tikbuf));
		
		ISFS_Close(ret);
		
		ISFS_Deinitialize();
	}
	
	if (buggycIOS) IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
	
	//Identify as channel.
	res = ES_Identify((signed_blob*)menu_certs_bin, menu_certs_bin_size, (signed_blob*)tmdbuf, tmdSize, (signed_blob*)tikbuf, tikSize, NULL);
	if(res < 0){
		return 0;
	}
	
	if (!channelidentified)
	{
		//Get current title ID.
		ES_GetTitleID(&titleID);
		if(titleID != channeltoload){
			return 0;
		}
		
		//Get tmd size.
		res = ES_GetStoredTMDSize(titleID, &tmdSize);
		if(res < 0){
			return 0;
		}
		
		//Get tmd.
		ptmd = memalign(32, tmdSize);
		res = ES_GetStoredTMD(titleID, ptmd, tmdSize);
		if(res < 0){
			return 0;
		}
		
		if (buggycIOS) IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
		
		//Identify as channel.
		res = ES_Identify((signed_blob*)menu_certs_bin, menu_certs_bin_size, ptmd, tmdSize, (signed_blob*)tikbuf, tikSize, NULL);
		if(res < 0){
			return 0;
		}
		
		ptmd = (signed_blob*)tmdbuf;
		channelios = ((tmd *)SIGNATURE_PAYLOAD(ptmd))->sys_version & 0xFF;
		bootindex = ((tmd *)SIGNATURE_PAYLOAD(ptmd))->boot_index;
		for (i = 0; i < ((tmd *)SIGNATURE_PAYLOAD(ptmd))->num_contents; i++)
		{
			if (((tmd *)SIGNATURE_PAYLOAD(ptmd))->contents[i].index == bootindex)
				bootid = ((tmd *)SIGNATURE_PAYLOAD(ptmd))->contents[i].cid;
		}
		channelidentified = 1;
	}
	
	return 1;
}

s32 ES_Load_dol(u16 index)
{
	u32 i;
	s32 ret;
	
	static dolheader dolfile[1] ATTRIBUTE_ALIGN(32);
	
	ret = ES_OpenContent(index);
	if(ret < 0){
		return 0;
	}
	
	ES_ReadContent(ret, (void *)dolfile, sizeof(dolheader));
	
	bootentrypoint = dolfile->entry_point;
	
	memset((void *)dolfile->bss_start, 0, dolfile->bss_size);
	
	for (i = 0; i < 7; i++)
	{
		if(dolfile->data_start[i] < sizeof(dolheader))
			continue;
		
		ES_SeekContent(ret, dolfile->text_pos[i], 0);
		ES_ReadContent(ret, (void *)dolfile->text_start[i], dolfile->text_size[i]);
		DCFlushRange((void *)dolfile->text_start[i], dolfile->text_size[i]);
	}
	
	for(i = 0; i < 11; i++)
	{
		if(dolfile->data_start[i] < sizeof(dolheader))
			continue;
		
		ES_SeekContent(ret, dolfile->data_pos[i], 0);
		ES_ReadContent(ret, (void *)dolfile->data_start[i], dolfile->data_size[i]);
		DCFlushRange((void *)dolfile->data_start[i], dolfile->data_size[i]);
	}
	
	ES_CloseContent(ret);	
	return 1;
}

s32 ISFS_Load_dol(u32 cid, u64 titleid)
{
	u32 i;
	s32 ret;
	char filename[256];
	
	static dolheader dolfile[1] ATTRIBUTE_ALIGN(32);
	
	snprintf(filename, sizeof(filename), "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(titleid), TITLE_LOWER(titleid), cid);
	
	if (ISFS_Initialize())
		return 0;
	
	ret = ISFS_Open(filename, ISFS_OPEN_READ);
	if(ret < 0){
		ISFS_Deinitialize();
		return 0;
	}
	
	ISFS_Read(ret, (void *)dolfile, sizeof(dolheader));
	
	bootentrypoint = dolfile->entry_point;
	
	memset((void *)dolfile->bss_start, 0, dolfile->bss_size);
	
	for (i = 0; i < 7; i++)
	{
		if(dolfile->data_start[i] < sizeof(dolheader))
			continue;
		
		ISFS_Seek(ret, dolfile->text_pos[i], 0);
		ISFS_Read(ret, (void *)dolfile->text_start[i], dolfile->text_size[i]);
		DCFlushRange((void *)dolfile->text_start[i], dolfile->text_size[i]);
	}
	
	for(i = 0; i < 11; i++)
	{
		if(dolfile->data_start[i] < sizeof(dolheader))
			continue;
		
		ISFS_Seek(ret, dolfile->data_pos[i], 0);
		ISFS_Read(ret, (void *)dolfile->data_start[i], dolfile->data_size[i]);
		DCFlushRange((void *)dolfile->data_start[i], dolfile->data_size[i]);
	}
	
	ISFS_Close(ret);
	
	ISFS_Deinitialize();
	
	return 1;
}
