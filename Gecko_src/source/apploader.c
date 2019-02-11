#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gccore.h>
#include <ogcsys.h>
#include <malloc.h>
#include <wiiuse/wpad.h>
#include <sys/unistd.h>
#include <ogc/lwp_watchdog.h>
#include <sys/dir.h>
#include <ogc/lwp.h>
#include <time.h>
#include <fat.h>

#include "tools.h"
#include "menu.h"
#include "dvd_broadway.h"
#include "multidol.h"
#include "codehandler.h"
#include "codehandlerslota.h"
#include "codehandleronly.h"
#include "defaultgameconfig.h"
#include "patchcode.h"
#include "apploader.h"
#include "sdcard/wiisd_io.h"
#include "sd.h"
#include "gfx.h"
#include "fst.h"
#include "identify.h"
#include "main.h"

#define PATCHDIR		"sd:/patch"
#define GECKOPATCHDIR	"sd:/data/gecko/patch"
#define CODEDIR			"sd:/codes"
#define GECKOCODEDIR	"sd:/data/gecko/codes"
#define GAMECONFIG		"sd:/gameconfig.txt"
#define GECKOGAMECONFIG	"sd:/data/gecko/gameconfig.txt"
#define DIAGFILE		"sd:/diagnostic.txt"
#define appbuffer 0x80100000
#define sdbuffer 0x90080000
#define BC 		0x0000000100000100ULL
#define MIOS 	0x0000000100000101ULL

u8 iosversion;
u8 fakeiosversion;
u8 no_patches;
u32 patch_dest;
u32 patch_size;
int app_running = 0;
int app_thread_state = 0;\
lwp_t thread;
struct wadinfo *ioswad = NULL;
char searchIOS[8];
u8 *tempcodelist = (u8 *) sdbuffer;
u8 *codelist = NULL;
u8 *codelistend = (u8 *) 0x80003000;
u32 codelistsize = 0;
int codes_state = 0;
int patch_state = 0;
char *tempgameconf = (char *) sdbuffer;
u32 *gameconf = NULL;
u32 tempgameconfsize = 0;
u32 gameconfsize = 0;
u32 customhook[8];
u8 customhooksize;
u8 willswitchios = 1;
u8 configwarn = 0;
u8 diagcreate = 0;
u8 applyfwritepatch = 0;
u8 dumpmaindol = 0;
u8 vipatchon;
int app_x_screen = 110*2;
int app_y_screen;
char gameidbuffer[8];
char filepath[MAX_FILEPATH_LEN];
GXRModeObj *rmode = NULL;
vu32 dvddone = 0;
u8 dvdios;
u32 appentrypoint;

dvddiskid *g_diskID = (dvddiskid*)0x80000000;
char _gameTocBuffer[0x20] ATTRIBUTE_ALIGN(32);
char _gamePartInfo[0x20] ATTRIBUTE_ALIGN(32);
char _gameTmdBuffer[18944] ATTRIBUTE_ALIGN(32);
u32 *__dvd_Tmd = NULL;
struct _toc *__dvd_gameToc = NULL;
struct _pinfo *__dvd_partInfo = NULL;
struct _pinfo *__dvd_bootGameInfo = NULL;

typedef void (*execute_patch_function) (void);

//---------------------------------------------------------------------------------
void nothing()
//---------------------------------------------------------------------------------
{
}

void __dvd_readidcb(s32 result)
{
	dvddone = result;
}

//---------------------------------------------------------------------------------
void app_apply_patch()
//---------------------------------------------------------------------------------
{

	int i;

	u8 *filebuff = (u8*)sdbuffer;
	u8 *asmbuff;

	no_patches = filebuff[0];

	filebuff += 1;

	for(i=0;i<no_patches;i++)
	{
		patch_dest = be32(filebuff);
		patch_size = be32(filebuff+4);

		memcpy((u8*)patch_dest, filebuff+8, patch_size);
		DCFlushRange((u8*)patch_dest, patch_size);
		filebuff = filebuff + patch_size + 8;
	}
}

//---------------------------------------------------------------------------------
void app_dopatch(char *filename)
//---------------------------------------------------------------------------------
{
	
	FILE* fp;
	u32 ret, pathlen;
	u32 filesize;

	DIR* pdir = opendir("/data/gecko/patch/");
	if(pdir == NULL){
		pdir = opendir ("/patch/");
		if(pdir == NULL){
			app_thread_state = 9;	// dir not found
			channel_thread_state = 4;
			codes_state = 0;
			while(1);
		}
	}

	closedir(pdir);
	fflush(stdout);
	
	sprintf(filepath, GECKOPATCHDIR "/%s.gpf", filename);
	
	fp = fopen(filepath, "rb");
	if (!fp) {
		sprintf(filepath, PATCHDIR "/%s.gpf", filename);
		
		fp = fopen(filepath, "rb");
		if (!fp) {
			app_thread_state = 10;	// Patch not found
			channel_thread_state = 5;
			codes_state = 0;
		while(1);
		}
	}

	fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

	ret = fread((void*)sdbuffer, 1, filesize, fp);
	if(ret != filesize){	
		fclose(fp);
		app_thread_state = 11;	// Patch not found
		channel_thread_state = 6;
		codes_state = 0;
		while(1);
	}
	tempcodelist += filesize;
	tempgameconf = tempcodelist;

	DCFlushRange((void*)sdbuffer, filesize);
}

//---------------------------------------------------------------------------------
void app_copycodes(char *filename)
//---------------------------------------------------------------------------------
{
	
	if (config_bytes[2] == 0x00)
	{
		codes_state = 3;
		return;
	}
	
	FILE* fp;
	u32 ret, pathlen;
	u32 filesize;
	
	DIR* pdir = opendir ("/data/gecko/codes/");
	if(pdir == NULL){
		pdir = opendir ("/codes/");
		if(pdir == NULL){
			codes_state = 1;	// dir not found
			return;
		}
	}
	
	closedir(pdir);
	fflush(stdout);
	
	sprintf(filepath, GECKOCODEDIR "/%s.gct", filename);
	
	fp = fopen(filepath, "rb");
	if (!fp) {
		sprintf(filepath, CODEDIR "/%s.gct", filename);
		
		fp = fopen(filepath, "rb");
		if (!fp) {
			codes_state = 1;	// codes not found
			return;
		}
	}
	
	fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
	
	codelistsize = filesize;
	
	if ((codelist + codelistsize) > codelistend)
	{
		fclose(fp);
		codes_state = 4;
		return;
	}
	
	ret = fread((void*)tempcodelist, 1, filesize, fp);
	if(ret != filesize){	
		fclose(fp);
		codelistsize = 0;
		codes_state = 1;	// codes not found
		return;
	}
	
	fclose(fp);
	DCFlushRange((void*)tempcodelist, filesize);
	
	codes_state = 2;
}

//---------------------------------------------------------------------------------
void app_loadgameconfig(char *gameid)
//---------------------------------------------------------------------------------
{
	FILE* fp;
	u32 ret;
	u32 filesize;
	s32 gameidmatch, maxgameidmatch = -1, maxgameidmatch2 = -1;
	u32 i, numnonascii, parsebufpos;
	u32 codeaddr, codeval, codeaddr2, codeval2, codeoffset;
	u32 temp, tempoffset, hookset = 0;
	char parsebuffer[18];
	
	if (config_bytes[2] == 8)
		hookset = 1;
	
	memcpy(tempgameconf, defaultgameconfig, defaultgameconfig_size);
	tempgameconf[defaultgameconfig_size] = '\n';
	tempgameconfsize = defaultgameconfig_size + 1;
	
	if (sd_found == 1)
	{
		fp = fopen(GECKOGAMECONFIG, "rb");
		
		if (!fp) fp = fopen(GAMECONFIG, "rb");
		
		if (fp) {
			fseek(fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			
			ret = fread((void*)tempgameconf + tempgameconfsize, 1, filesize, fp);
			fclose(fp);
			if (ret == filesize)
				tempgameconfsize += filesize;
		}
	}
	
	// Remove non-ASCII characters
	numnonascii = 0;
	for (i = 0; i < tempgameconfsize; i++)
	{
		if (tempgameconf[i] < 9 || tempgameconf[i] > 126)
			numnonascii++;
		else
			tempgameconf[i-numnonascii] = tempgameconf[i];
	}
	tempgameconfsize -= numnonascii;
	
	*(tempgameconf + tempgameconfsize) = 0;
	gameconf = (tempgameconf + tempgameconfsize) + (4 - (((u32) (tempgameconf + tempgameconfsize)) % 4));
	
	for (maxgameidmatch = 0; maxgameidmatch <= 6; maxgameidmatch++)
	{
		i = 0;
		while (i < tempgameconfsize)
		{
			maxgameidmatch2 = -1;
			while (maxgameidmatch != maxgameidmatch2)
			{
				while (i != tempgameconfsize && tempgameconf[i] != ':') i++;
				if (i == tempgameconfsize) break;
				while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
				if (i != 0) i++;
				parsebufpos = 0;
				gameidmatch = 0;
				while (tempgameconf[i] != ':')
				{
					if (tempgameconf[i] == '?')
					{
						parsebuffer[parsebufpos] = gameid[parsebufpos];
						parsebufpos++;
						gameidmatch--;
						i++;
					}
					else if (tempgameconf[i] != 0 && tempgameconf[i] != ' ')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ')
						break;
					else
						i++;
					if (parsebufpos == 8) break;
				}
				parsebuffer[parsebufpos] = 0;
				if (strncasecmp("DEFAULT", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 7)
				{
					gameidmatch = 0;
					goto idmatch;
				}
				if (strncmp(gameid, parsebuffer, strlen(parsebuffer)) == 0)
				{
					gameidmatch += strlen(parsebuffer);
				idmatch:
					if (gameidmatch > maxgameidmatch2)
					{
						maxgameidmatch2 = gameidmatch;
					}
				}
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
			}
			while (i != tempgameconfsize && tempgameconf[i] != ':')
			{
				parsebufpos = 0;
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
				{
					if (tempgameconf[i] != 0 && tempgameconf[i] != ' ' && tempgameconf[i] != '(' && tempgameconf[i] != ':')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ' || tempgameconf[i] == '(' || tempgameconf[i] == ':')
						break;
					else
						i++;
					if (parsebufpos == 17) break;
				}
				parsebuffer[parsebufpos] = 0;
				if (!autobootcheck)
				{
					//if (strncasecmp("addtocodelist(", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 14)
					//{
					//	ret = sscanf(tempgameconf + i, "%x %x", &codeaddr, &codeval);
					//	if (ret == 2)
					//		addtocodelist(codeaddr, codeval);
					//}
					if (strncasecmp("codeliststart", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
					{
						sscanf(tempgameconf + i, " = %x", &codelist);
					}
					if (strncasecmp("codelistend", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						sscanf(tempgameconf + i, " = %x", &codelistend);
					}
					if (strncasecmp("hooktype", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						if (hookset == 1)
						{
							ret = sscanf(tempgameconf + i, " = %u", &temp);
							if (ret == 1)
								if (temp >= 0 && temp <= 7)
									config_bytes[2] = temp;
						}
					}
					if (strncasecmp("poke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
					{
						ret = sscanf(tempgameconf + i, "( %x , %x", &codeaddr, &codeval);
						if (ret == 2)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = NULL;
							gameconfsize += 8;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("pokeifequal", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, "( %x , %x , %x , %x", &codeaddr, &codeval, &codeaddr2, &codeval2);
						if (ret == 4)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr2;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval2;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("searchandpoke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
					{
						ret = sscanf(tempgameconf + i, "( %x%n", &codeval, &tempoffset);
						if (ret == 1)
						{
							gameconfsize += 4;
							temp = 0;
							while (ret == 1)
							{
								*(gameconf + (gameconfsize / 4)) = codeval;
								gameconfsize += 4;
								temp++;
								i += tempoffset;
								ret = sscanf(tempgameconf + i, " %x%n", &codeval, &tempoffset);
							}
							*(gameconf + (gameconfsize / 4) - temp - 1) = temp;
							ret = sscanf(tempgameconf + i, " , %x , %x , %x , %x", &codeaddr, &codeaddr2, &codeoffset, &codeval2);
							if (ret == 4)
							{
								*(gameconf + (gameconfsize / 4)) = codeaddr;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeaddr2;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeoffset;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeval2;
								gameconfsize += 4;
								DCFlushRange((void *) (gameconf + (gameconfsize / 4) - temp - 5), temp * 4 + 20);
							}
							else
								gameconfsize -= temp * 4 + 4;
						}
						
					}
					if (strncasecmp("hook", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
					{
						ret = sscanf(tempgameconf + i, "( %x %x %x %x %x %x %x %x", customhook, customhook + 1, customhook + 2, customhook + 3, customhook + 4, customhook + 5, customhook + 6, customhook + 7);
						if (ret >= 3)
						{
							if (hookset != 1)
								configwarn |= 4;
							config_bytes[2] = 0x08;
							customhooksize = ret * 4;
						}
					}
					if (strncasecmp("002fix", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 6)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 0x1)
								fakeiosversion = temp;
					}
					if (strncasecmp("switchios", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 9)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								willswitchios = temp;
					}
					if (strncasecmp("videomode", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 9)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0)
							{
								if (config_bytes[1] != 0x00)
									configwarn |= 1;
								config_bytes[1] = 0x00;
							}
							else if (temp == 1)
							{
								if (config_bytes[1] != 0x03)
									configwarn |= 1;
								config_bytes[1] = 0x03;
							}
							else if (temp == 2)
							{
								if (config_bytes[1] != 0x01)
									configwarn |= 1;
								config_bytes[1] = 0x01;
							}
							else if (temp == 3)
							{
								if (config_bytes[1] != 0x02)
									configwarn |= 1;
								config_bytes[1] = 0x02;
							}
						}
					}
					if (strncasecmp("language", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0)
							{
								if (config_bytes[0] != 0xCD)
									configwarn |= 2;
								config_bytes[0] = 0xCD;
							}
							else if (temp > 0 && temp <= 10)
							{
								if (config_bytes[0] != temp-1)
									configwarn |= 2;
								config_bytes[0] = temp-1;
							}
						}
					}
					if (strncasecmp("diagnostic", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 10)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0 || temp == 1)
								diagcreate = temp;
						}
					}
					if (strncasecmp("vidtv", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 5)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								vipatchon = temp;
					}
					if (strncasecmp("fwritepatch", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								applyfwritepatch = temp;
					}
					if (strncasecmp("dumpmaindol", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								dumpmaindol = temp;
					}
				}
				else
				{
					if (strncasecmp("autoboot", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								autoboot = temp;
					}
					if (strncasecmp("autobootwait", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 12)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 255)
								autobootwait = temp;
					}
					if (strncasecmp("autoboothbc", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								autoboothbc = temp;
					}
					if (strncasecmp("autobootocarina", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 15)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								config_bytes[4] = temp;
					}
					if (strncasecmp("autobootdebugger", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 16)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								config_bytes[7] = temp;
					}
					if (strncasecmp("rebootermenuitem", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 16)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								rebooterasmenuitem = temp;
					}
					if (strncasecmp("startupios", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 10)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 255)
							{
								sd_shutdown();
								IOS_ReloadIOS(temp);
								detectIOScapabilities();
								sd_init();
								startupiosloaded = 1;
							}
					}
				}
				if (tempgameconf[i] != ':')
				{
					while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
					if (i != tempgameconfsize) i++;
				}
			}
			if (i != tempgameconfsize) while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
		}
	}
	
	tempcodelist = ((u8 *) gameconf) + gameconfsize;
}

//---------------------------------------------------------------------------------
void app_pokevalues()
//---------------------------------------------------------------------------------
{
	u32 i, *codeaddr, *codeaddr2, *addrfound = NULL;
	
	if (gameconfsize != 0)
	{
		for (i = 0; i < (gameconfsize / 4); i++)
		{
			if (*(gameconf + i) == 0)
			{
				if (((u32 *) (*(gameconf + i + 1))) == NULL ||
					*((u32 *) (*(gameconf + i + 1))) == *(gameconf + i + 2))
				{
					*((u32 *) (*(gameconf + i + 3))) = *(gameconf + i + 4);
					DCFlushRange((void *) *(gameconf + i + 3), 4);
				}
				i += 4;
			}
			else
			{
				codeaddr = *(gameconf + i + *(gameconf + i) + 1);
				codeaddr2 = *(gameconf + i + *(gameconf + i) + 2);
				if (codeaddr == 0 && addrfound != NULL)
					codeaddr = addrfound;
				else if (codeaddr == 0 && codeaddr2 != 0)
					codeaddr = (u32 *) ((((u32) codeaddr2) >> 28) << 28);
				else if (codeaddr == 0 && codeaddr2 == 0)
				{
					i += *(gameconf + i) + 4;
					continue;
				}
				if (codeaddr2 == 0)
					codeaddr2 = codeaddr + *(gameconf + i);
				addrfound = NULL;
				while (codeaddr <= (codeaddr2 - *(gameconf + i)))
				{
					if (memcmp(codeaddr, gameconf + i + 1, (*(gameconf + i)) * 4) == 0)
					{
						*(codeaddr + ((*(gameconf + i + *(gameconf + i) + 3)) / 4)) = *(gameconf + i + *(gameconf + i) + 4);
						if (addrfound == NULL) addrfound = codeaddr;
					}
					codeaddr++;
				}
				i += *(gameconf + i) + 4;
			}
		}
	}
}

/* display_tmd_info and some of app_writediag is based on bushing's
   title lister code; see the below notice */

/*-------------------------------------------------------------
 
 Copyright (C) 2008 bushing
 
 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any
 damages arising from the use of this software.
 
 Permission is granted to anyone to use this software for any
 purpose, including commercial applications, and to alter it and
 redistribute it freely, subject to the following restrictions:
 
 1.The origin of this software must not be misrepresented; you
 must not claim that you wrote the original software. If you use
 this software in a product, an acknowledgment in the product
 documentation would be appreciated but is not required.
 
 2.Altered source versions must be plainly marked as such, and
 must not be misrepresented as being the original software.
 
 3.This notice may not be removed or altered from any source
 distribution.
 
 -------------------------------------------------------------*/

char * display_tmd_info(const tmd *t) {
	static char desc[256];
	u32 kind = t->title_id >> 32;
	u32 title_l = t->title_id & 0xFFFFFFFF;
	
	char title_ascii[5];
	u32 i;
	memcpy(title_ascii, &title_l, 4);
	for (i=0; i<4; i++) title_ascii[i]=ascii(title_ascii[i]);
	title_ascii[4]='\0';
	
	switch (kind) {
		case 1: // IOS, MIOS, BC, System Menu
			snprintf(desc, sizeof(desc), "Title=1-%x (", title_l);
			switch(title_l) {
				case 2:   strlcat(desc, "System Menu) ", sizeof(desc)); break;
				case 0x100: strlcat(desc, "BC) ", sizeof(desc)); break;
				case 0x101: strlcat(desc, "MIOS) ", sizeof(desc)); break;
				default:  sprintf(desc + strlen(desc), "IOS%d) ", title_l); break;
			}
			break;
		case 0x10000: // TMD installed by running a disc
			snprintf(desc, sizeof(desc), "Title=10000-%08x (savedata for '%s')", 
					 title_l, title_ascii); break;
		case 0x10001: // Normal channels / VC
			snprintf(desc, sizeof(desc), "Title=10001-%08x (downloaded channel '%s')",
					 title_l, title_ascii); break;
		case 0x10002: // "System channels" -- News, Weather, etc.
			snprintf(desc, sizeof(desc), "Title=10002-%08x (system channel '%s')", 
					 title_l, title_ascii); break;
		case 0x10004: // "Hidden channels" -- WiiFit channel
			snprintf(desc, sizeof(desc), "Title=10004-%08x (game channel '%s')",
					 title_l, title_ascii); break;
		case 0x10008: // "Hidden channels" -- EULA, rgnsel
			snprintf(desc, sizeof(desc), "Title=10008-%08x (hidden? channel '%s')", 
					 title_l, title_ascii); break;
		default:
			printf("Unknown title type %x %08x\n", kind, title_l);
			break;
	}
    
	if (t->title_version) 
		snprintf(desc, sizeof(desc), "%s vers: %d.%d (%d)", desc, t->title_version >> 8, t->title_version & 0xFF,
				 t->title_version);
	if (t->sys_version)   snprintf(desc, sizeof(desc), "%s FW: IOS%llu ", desc, t->sys_version & 0xff);

  return desc;
}

//---------------------------------------------------------------------------------
static void app_writediag(void *app_init, void *app_main, void *app_final, void *app_entry)
//---------------------------------------------------------------------------------
{
	char *desc;
	FILE* fp;
	s32 ret;
	u32 count;
	static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	signed_blob *s_tmd;
	u32 tmd_size;
	u32 i;
	
	if (sd_found == 1)
	{
		fp = fopen(DIAGFILE, "w");
		if (fp) {
			fprintf(fp, "Game ID: %s\n", gameidbuffer);
			fprintf(fp, "Required IOS: %u\n", dvdios);
			for (i = 0; i < 12; i++)
				fprintf(fp, "config_bytes[%u] = 0x%02X\n", i, config_bytes[i]);
			fprintf(fp, "app_init: %x\napp_main: %x\napp_final: %x\napp_entry: %x\n\n", (u32) app_init, (u32) app_main, (u32) app_final, (u32) app_entry);
			fprintf(fp, "Wii Titles:\n");
			ret = ES_GetNumTitles(&count);
			if (ret) {
				fprintf(fp, "ES_GetNumTitles=%d, count=%08x\n", ret, count);
				return;
			}
			
			fprintf(fp, "Found %d titles\n", count);
			
			static u64 title_list[256] ATTRIBUTE_ALIGN(32);
			ret = ES_GetTitles(title_list, count);
			if (ret) {
				fprintf(fp, "ES_GetTitles=%d\n", ret);
				return;
			}
			
			int i;
			for (i=0; i < count; i++) {
				ret = ES_GetStoredTMDSize(title_list[i], &tmd_size);
				s_tmd = (signed_blob *)tmd_buf;
				ret = ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);
				desc = display_tmd_info(SIGNATURE_PAYLOAD(s_tmd));
				fprintf(fp, "%d: %s\n", i+1, desc);
			}
		}
		fprintf(fp, "\n<gameconfig.txt>\n");
		fwrite(tempgameconf, 1, tempgameconfsize, fp);
		fclose(fp);
	}
}

//---------------------------------------------------------------------------------
void set_default_vidmode()
//---------------------------------------------------------------------------------
{
	switch(gameidbuffer[3])
	{
		case 0x50:		// PAL default
		case 0x44:		// PAL German
		case 0x46:		// F (french PAL)
		case 0x58:		// PAL X euro
		case 0x59:		// PAL Y euro
			*(vu32*)0x800000CC = 1; 
			rmode = &TVPal528IntDf;
		break;

		case 0x45:		// USA default
		case 0x4A:		// JAP default
			*(vu32*)0x800000CC = 0; 
			rmode = &TVNtsc480IntDf;
		break;
	}
}

//---------------------------------------------------------------------------------
void process_config_vidmode()
//---------------------------------------------------------------------------------
{
	switch(config_bytes[1])
	{
		case 0x00:						// default (no change)
		break;

		case 0x01:
			rmode =	&TVEurgb60Hz480Int; // PAL60 forced
			*(u32*)0x800000CC = 1;		// color fix, values just reverse
		break;

		case 0x02:
			rmode = &TVPal528IntDf;		// PAL50 forced
			*(u32*)0x800000CC = 1;		// color fix, values just reverse
		break;

		case 0x03:
			rmode = &TVNtsc480IntDf;	// NTSC forced
			*(u32*)0x800000CC = 0;		// color fix, values just reverse
		break;
	}

	
}

//---------------------------------------------------------------------------------
void load_handler()
//---------------------------------------------------------------------------------
{
	if (config_bytes[2] != 0x00)
	{
		if (config_bytes[7] == 0x01)
		{
			switch(gecko_channel)
			{
				case 0: // Slot A
					memset((void*)0x80001800,0,codehandlerslota_size);
					memcpy((void*)0x80001800,codehandlerslota,codehandlerslota_size);
					if (config_bytes[5] == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F5A, &codelist, 2);
					memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandlerslota_size);
					break;
					
				case 1:	// slot B
					memset((void*)0x80001800,0,codehandler_size);
					memcpy((void*)0x80001800,codehandler,codehandler_size);
					if (config_bytes[5] == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F5A, &codelist, 2);
					memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandler_size);
					break;
					
				case 2:
					memset((void*)0x80001800,0,codehandler_size);
					memcpy((void*)0x80001800,codehandler,codehandler_size);
					if (config_bytes[5] == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F5A, &codelist, 2);
					memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandler_size);
					break;
			}
		}
		else
		{
			memset((void*)0x80001800,0,codehandleronly_size);
			memcpy((void*)0x80001800,codehandleronly,codehandleronly_size);
			memcpy((void*)0x80001906, &codelist, 2);
			memcpy((void*)0x8000190A, ((u8*) &codelist) + 2, 2);
			DCFlushRange((void*)0x80001800,codehandleronly_size);
		}
		// Load multidol handler
		memset((void*)0x80001000,0,multidol_size);
		memcpy((void*)0x80001000,multidol,multidol_size); 
		DCFlushRange((void*)0x80001000,multidol_size);
		switch(config_bytes[2])
		{
			case 0x01:
				memcpy((void*)0x8000119C,viwiihooks,12);
				memcpy((void*)0x80001198,viwiihooks+3,4);
				break;
			case 0x02:
				memcpy((void*)0x8000119C,kpadhooks,12);
				memcpy((void*)0x80001198,kpadhooks+3,4);
				break;
			case 0x03:
				memcpy((void*)0x8000119C,joypadhooks,12);
				memcpy((void*)0x80001198,joypadhooks+3,4);
				break;
			case 0x04:
				memcpy((void*)0x8000119C,gxdrawhooks,12);
				memcpy((void*)0x80001198,gxdrawhooks+3,4);
				break;
			case 0x05:
				memcpy((void*)0x8000119C,gxflushhooks,12);
				memcpy((void*)0x80001198,gxflushhooks+3,4);
				break;
			case 0x06:
				memcpy((void*)0x8000119C,ossleepthreadhooks,12);
				memcpy((void*)0x80001198,ossleepthreadhooks+3,4);
				break;
			case 0x07:
				memcpy((void*)0x8000119C,axnextframehooks,12);
				memcpy((void*)0x80001198,axnextframehooks+3,4);
				break;
			case 0x08:
				if (customhooksize == 16)
				{
					memcpy((void*)0x8000119C,customhook,12);
					memcpy((void*)0x80001198,customhook+3,4);
				}
				break;
			case 0x09:
				memcpy((void*)0x8000119C,wpadbuttonsdownhooks,12);
				memcpy((void*)0x80001198,wpadbuttonsdownhooks+3,4);
				break;
			case 0x0A:
				memcpy((void*)0x8000119C,wpadbuttonsdown2hooks,12);
				memcpy((void*)0x80001198,wpadbuttonsdown2hooks+3,4);
				break;
		}
		DCFlushRange((void*)0x80001198,16);
	}
	if (codes_state == 2)
	{
		memcpy(codelist,tempcodelist,codelistsize);
		DCFlushRange(codelist,codelistsize);
	}
}

//---------------------------------------------------------------------------------
u32 dvd_switchios()
//---------------------------------------------------------------------------------
{
	void (*app_init)(void (*report)(const char* fmt, ...));
	int (*app_main)(void** dst, int* size, int* offset);
	void* (*app_final)();
	void (*app_entry)(void(**init)(void (*report)(const char* fmt, ...)),
	int (**main)(), void *(**final)());
	char* buffer = (char*)appbuffer;
	char filepath[256];
	s32 ret = 0;
	int i = 0;
	FILE *fp = NULL;
	void *maindolend = 0x80000000;
	
	app_y_screen = 115;

	app_thread_state = 1;	// setting up drive text

	ret = DVDLowInit();
	dvddone = 0;
	ret = DVDLowReset(__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	
	memset((char*)0x80000000, 0, 0x20);
	
	// Get the ID
	dvddone = 0;
	ret = DVDLowReadID(g_diskID,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	memset(gameidbuffer, 0, 8);
	memcpy(gameidbuffer, (char*)0x80000000, 6);	

	if(gameidbuffer[1] == 0 && gameidbuffer[2] == 0 && gameidbuffer[3] == 0 && gameidbuffer[4] == 0 && gameidbuffer[5] == 0 && gameidbuffer[6] == 0){
		app_thread_state = 2;	// Reading DVD Stats
		while(1);
	}

	if(*((u32 *) 0x8000001C) == 0xC2339F3D){
		set_default_vidmode();
		process_config_vidmode();
		WII_LaunchTitle(BC);
	}
	else if (*((u32 *) 0x80000018) != 0x5D1C9EA3)
	{
		app_thread_state = 13;
		while(1);
	}

	app_thread_state = 3;	// Reading DVD Stats

	dvddone = 0;
	__dvd_gameToc = (struct _toc*)_gameTocBuffer;
	DVDLowUnencryptedRead(_gameTocBuffer,0x20,0x00010000,__dvd_readidcb);
	while(dvddone==0);

	dvddone = 0;
	__dvd_partInfo = (struct _pinfo*)_gamePartInfo;
	DVDLowUnencryptedRead(_gamePartInfo,0x20,__dvd_gameToc->partInfoOff,__dvd_readidcb);
	while(dvddone==0);

	i = 0;
	__dvd_bootGameInfo = NULL;

	while(i<__dvd_gameToc->bootInfoCnt) {
		if(__dvd_partInfo[i].len==0) {
			__dvd_bootGameInfo = &__dvd_partInfo[i];
		}
		i++;
	}

	dvddone = 0;
	__dvd_Tmd = (u32*)_gameTmdBuffer;
	 		
	ret = DVDLowOpenPartition(__dvd_bootGameInfo->offset,NULL,0,NULL,(void*)_gameTmdBuffer,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	dvdios = _gameTmdBuffer[0x18B];

	dvddone = 0;
	ret = DVDLowClosePartition(__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	tempcodelist = (u8 *) sdbuffer;
	codelistsize = 0;
	tempgameconf = (char *) sdbuffer;
	gameconfsize = 0;
	vipatchon = 0;
	applyfwritepatch = 0;
	dumpmaindol = 0;
	
	if (config_bytes[7] == 0x00)
		codelist = (u8 *) 0x800022A8;
	else
		codelist = (u8 *) 0x800028B8;
	
	// Need to load patch to high mem but not apply
	if(sd_found == 1 && config_bytes[3] == 0x01){		
		app_dopatch(gameidbuffer);
	}
	
	app_loadgameconfig(gameidbuffer);
	
	// Need to load codes to high mem but not apply
	if(sd_found == 1 && config_bytes[4] == 0x01){
		app_copycodes(gameidbuffer);
	}
	
	WPAD_Shutdown();
	
	if ((IOS_GetVersion() == dvdios || !willswitchios || diagcreate || dumpmaindol) && dvdios != 249)
		app_thread_state = 5;
	else
	{
		fatUnmount("sd");
		if (dvdios == 249) dvdios = 36;
		ret = IOS_ReloadIOS(dvdios);
		if(ret < 0){
			i = 0;
			__dvd_bootGameInfo = NULL;
			
			while(i<__dvd_gameToc->bootInfoCnt) {
				if(__dvd_partInfo[i].len==1) {	// 1 for update partition
					__dvd_bootGameInfo = &__dvd_partInfo[i];
				}
				i++;
			}
			
			if (__dvd_bootGameInfo == NULL)
				goto returntomenu;
			
			dvddone = 0;
			__dvd_Tmd = (u32*)_gameTmdBuffer;
			ret = DVDLowOpenPartition(__dvd_bootGameInfo->offset,NULL,0,NULL,(void*)_gameTmdBuffer,__dvd_readidcb);
			while(ret>=0 && dvddone==0);
			
			dvddone = 0;
			ret = DVDLowRead(buffer,0x20,0x2440/4,__dvd_readidcb);
			while(ret>=0 && dvddone==0);
			DCFlushRange(buffer, 0x20);
			
			dvddone = 0;
			ret = DVDLowRead((void*)0x81200000,((*(u32*)(buffer + 0x14)) + 31) & ~31,0x2460/4,__dvd_readidcb);
			while(ret>=0 && dvddone==0);
			
			app_entry = (void (*)(void(**)(void (*)(const char*, ...)), int (**)(), void *(**)()))(*(u32*)(buffer + 0x10));
			app_entry(&app_init, &app_main, &app_final);
			app_init((void (*)(const char*, ...))nothing);
			
			while (1)
			{
				void* dst = 0;
				int len = 0,offset = 0;
				int res = app_main(&dst, &len, &offset);
				
				if (!res){
					break;
				}
				
				dvddone = 0;
				ret = DVDLowRead(dst,len,offset/4<<2,__dvd_readidcb);
				while(ret>=0 && dvddone==0);
				DCFlushRange(dst, len);
				
			}
			
			sprintf(searchIOS,"IOS%u-",dvdios);
			ioswad = fst_find_wad(*(u32*)0x80000038, searchIOS);
		returntomenu:
			ret = sd_init();
			if(!ret){
				sd_found = 0;
			}
			else {
				sd_found = 1;
			}
			codes_state = 0;
			configwarn = 0;
			app_thread_state = 4;	// Reading DVD Stats
			while(1);
		}
		
		app_thread_state = 5;	// Reading DVD Stats
		
		ret = DVDLowInit();
		
		memset((char*)0x80000000, 0, 6);
		
		dvddone = 0;
		ret = DVDLowReadID(g_diskID,__dvd_readidcb);
		while(ret>=0 && dvddone==0);
		
		memset(gameidbuffer, 0, 8);
		memcpy(gameidbuffer, (char*)0x80000000, 6);
		
		if(gameidbuffer[1] == 0 && gameidbuffer[2] == 0 && gameidbuffer[3] == 0 && gameidbuffer[4] == 0 && gameidbuffer[5] == 0 && gameidbuffer[6] == 0){
			app_thread_state = 12;	// Reading DVD Stats
			codes_state = 0;
			while(1);
		}
	}

	dvddone = 0;
	__dvd_gameToc = (struct _toc*)_gameTocBuffer;
	DVDLowUnencryptedRead(_gameTocBuffer,0x20,0x00010000,__dvd_readidcb);
	while(dvddone==0);

	dvddone = 0;
	__dvd_partInfo = (struct _pinfo*)_gamePartInfo;
	DVDLowUnencryptedRead(_gamePartInfo,0x20,__dvd_gameToc->partInfoOff,__dvd_readidcb);
	while(dvddone==0);

	i = 0;
	__dvd_bootGameInfo = NULL;

	while(i<__dvd_gameToc->bootInfoCnt) {
		if(__dvd_partInfo[i].len==0) {
			__dvd_bootGameInfo = &__dvd_partInfo[i];
		}
		i++;
	}

	dvddone = 0;
	__dvd_Tmd = (u32*)_gameTmdBuffer;
	 		
	ret = DVDLowOpenPartition(__dvd_bootGameInfo->offset,NULL,0,NULL,(void*)_gameTmdBuffer,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	dvddone = 0;
	ret = DVDLowRead(buffer,0x20,0x2440/4,__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	DCFlushRange(buffer, 0x20);

	dvddone = 0;
	ret = DVDLowRead((void*)0x81200000,((*(u32*)(buffer + 0x14)) + 31) & ~31,0x2460/4,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	app_entry = (void (*)(void(**)(void (*)(const char*, ...)), int (**)(), void *(**)()))(*(u32*)(buffer + 0x10));
	app_entry(&app_init, &app_main, &app_final);
	app_init((void (*)(const char*, ...))nothing);

	settime(secs_to_ticks(time(NULL) - 946684800));
	
	if (dumpmaindol)
	{
		sprintf(filepath, "sd:/%s_dol.bin", gameidbuffer);
		fp = fopen(filepath, "w+");
	}
	
	while (1)
	{
		void* dst = 0;
		int len = 0,offset = 0;
		int res = app_main(&dst, &len, &offset);
		
		if (!res){
			break;
		}

		dvddone = 0;
		ret = DVDLowRead(dst,len,offset/4<<2,__dvd_readidcb);
		while(ret>=0 && dvddone==0);
		DCFlushRange(dst, len);
		
		if (fp && dst >= 0x80000000 && dst <= 0x80dfff00)
			if (maindolend < (dst + len))
				maindolend = (void *) (dst + len);

		// VIDTV Patch
		if(vipatchon){
			vidolpatcher(dst,len);
		}
		
		// fwrite Patch
		if (applyfwritepatch){
			patchdebug(dst,len);
		}
		
		// Language
		if(config_bytes[0] != 0xCD){
			langpatcher(dst,len);
		}
		
		// Hooks
		if(config_bytes[2] != 0x00){
			dogamehooks(dst,len);
		}

		DCFlushRange(dst, len);
	}
	
	load_handler();
	app_pokevalues();

	// Do SD patch if sd card and enabled
	if(sd_found == 1 &&config_bytes[3] == 0x01){		
		app_apply_patch();
	}
	
	if (diagcreate)
	{
		app_writediag((void *) app_init, (void *) app_main, (void *) app_final, (void *) app_entry);
		if (!fp)
		{
			fatUnmount("sd");
			STM_RebootSystem();
		}
	}
	
	if (fp)
	{
		fwrite((void *) 0x80000000, ((u32) maindolend) - 0x80000000, 1, fp);
		fclose(fp);
		fatUnmount("sd");
		STM_RebootSystem();
	}

	set_default_vidmode();
	process_config_vidmode();
	
	if (progmode)
		rmode = &TVNtsc480Prog;

	VIDEO_Configure(rmode);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// process paused start
/*	switch(config_bytes[5])
	{
		case 0x00:						// No paused start (no change)
		break;

		case 0x01:
			write32(0x80002784, 1);
		break;		
	} */

	u32 arealow = *(u32*)0x80000034;
	u32 areahigh = *(u32*)0x80000038;
	u32 maxfst = *(u32*)0x8000003c;
	u32 bi2 = *(u32*)0x800000f4;
	
	*(u32*)0x80000020 = 0xD15EA5E;		// Boot from DVD
	*(u32*)0x80000024 = 1; 				// Version
	*(u32*)0x80000030 = 0; 				// Arena Low
	*(u32*)0x80000034 = arealow;		// Arena High - get from DVD
	*(u32*)0x80000038 = areahigh;		// FST Start - get from DVD
	*(u32*)0x8000003C = maxfst;			// Max FST size - get from DVD
	
	*(u32*)0x800000EC = 0x81800000;		// Dev Debugger Monitor Address
	*(u32*)0x800000F0 = 0x01800000;		// Dev Debugger Monitor Address
	*(u32*)0x800000F4 = bi2;			// BI2
	*(u32*)0x800000F8 = 0x0E7BE2C0;		// Console Bus Speed
	*(u32*)0x800000FC = 0x2B73A840;		// Console CPU Speed
	
	if (fakeiosversion)
		*(u16*)0x80003142 = 0xFFFF;

	memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
	memcpy((void*)0x80003180, (char*)0x80000000, 4);	// online check code, seems offline games clear it?
	
	DCFlushRange((void*)0x80000000, 0x3f00);
		
	appentrypoint = app_final();
	app_thread_state = 8;	// shut down

	while(1);
	return;
}

void apploader_thread_close()
{
	if(app_running){
		LWP_SuspendThread(thread);
		app_running = 0;
	}
}

void apploader_thread()
{
	app_thread_state = 0;

	if(!app_running){
		LWP_CreateThread(&thread, dvd_switchios, NULL, NULL, 0, 2);
		app_running = 1;
	}
}
