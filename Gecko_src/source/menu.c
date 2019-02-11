#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gccore.h>	
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include <sys/unistd.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/lwp.h>
#include <fat.h>
#include <sys/dir.h>
#include <malloc.h>

#include "sdcard/wiisd_io.h"
#include "gfx.h"
#include "menu.h"
#include "apploader.h"
#include "sd.h"
#include "fst.h"
#include "file.h"
#include "identify.h"
#include "sysmenu.h"
#include "tools.h"
#include "dvd_broadway.h"
#include "patchcode.h"
#include "main.h"
#include "fakesign.h"

#define maxchars 20
#define font_y_pos 110
#define font_x_pos 115*2
#define font_y_spacing 22
#define sdbuffer 0x90080000

int scanned = 0;
int idcount = 0;
int file_menu_selected = 0;
int config_not_loaded = 0;
u8 rebooterasmenuitem = 1;
int config_font_x_pos = 95*2;
int rebooterconf_font_x_pos = 95*2;
int tools_font_x_pos = 95*2;
int font_x_screen = font_x_pos;
int font_y_screen = font_y_pos;
int selected = font_y_pos;

u8 textfade = 250;
#define fadevalue 10
u32 fadedir;
u32 menu_number = 0;
u32 error_sd = 0;
u32 confirm_sd = 0;
u32 error_video = 0;

// Help
#define help_menu_items 1
int help_menu_selected = 0;
typedef struct{char *hitems; int selected;} helpitems;
helpitems helpmenu[]=
	{{"Gecko 1.9.3.1",0}};

// Root Menu
#define root_menu_items 7
int root_menu_selected = 0;
typedef struct{char *ritems; int selected;} rootitems;
rootitems rootmenu[]=
	{{"Launch Game",0},
	{"Launch Channel",0},
	{"Launch Rebooter",0},
	{"Config Options",0},
	{"Rebooter Options",0},
	{"About",0},
	{"Exit",0}};

// Config Menu
#define config_menu_items 11
int config_menu_selected = 0;
typedef struct{char *citems; int selected;} configitems;
configitems configmenu[]=
	{{"Game Language:",0},
	{"Force NTSC:",0},
	{"Force PAL60:",0},
	{"Force PAL50:",0},
	{"Gecko Hook Type:",0},
	{"Load Debugger:",0},
	{"SD File Patcher:",0},
	{"SD Cheats:",0},
	{"Gecko Pause Start:",0},
	{"Bubbles On:",0},
	{"Save Config",0}};

// Rebooter Config Menu
#define rebooterconf_menu_items 4
int rebooterconf_menu_selected = 0;
typedef struct{char *rcitems; int selected;} rebooterconfitems;
rebooterconfitems rebooterconfmenu[]=
	{{"Recovery Mode:",0},
	{"Region Free:",0},
	{"Remove Copy Flags:",0},
	{"Button Skip:",0}};

// Tools Menu
#define tools_menu_items 4
int tools_menu_selected = 0;
typedef struct{char *titems; int selected;} toolsitems;
toolsitems toolsmenu[]=
	{{"Refresh Devices",0},
	{"Extract Game Updates To SD",0},
	{"Install Game Updates From SD",0},
	{"Patch Datel(TM) GC Checks",0}};

// Channel List Menu
u16 channellist_menu_items = 0;
int channellist_menu_selected = 0;
int channellist_menu_top = 0;
typedef struct{char *clitems; int selected; u64 titleid;} channellistitems;
channellistitems channellistmenu[256];

// Save List Menu
u16 savelist_menu_items = 0;
int savelist_menu_selected = 0;
int savelist_menu_top = 0;
typedef struct{char *slitems; int selected; u64 titleid;} savelistitems;
savelistitems savelistmenu[256];

// SD Card Save List Menu
u16 cardsavelist_menu_items = 0;
int cardsavelist_menu_selected = 0;
int cardsavelist_menu_top = 0;
typedef struct{char *slitems; int selected; u64 titleid;} cardsavelistitems;
cardsavelistitems cardsavelistmenu[256];

u32 langselect = 0;
u32 ntscselect = 0;
u32 pal60select = 0;
u32 pal50select = 0;
u32 hooktypeselect = 1;
u32 debuggerselect = 1;
u32 filepatcherselect = 0;
u32 ocarinaselect = 0;
u32 pausedstartselect = 0;
u32 bubbleselect = 0;
u32 recoveryselect = 0;
u32 regionfreeselect = 0;
u32 nocopyselect = 0;
u32 buttonskipselect = 0;

char languages[11][22] = 
	{{"Default"},
	{"Japanese"},
	{"English"},
	{"German"},
	{"French"},
	{"Spanish"},
	{"Italian"},
	{"Dutch"},
	{"S. Chinese"},
	{"T. Chinese"},
	{"Korean"}};

char ntsc_options[2][6] = 
	{{"NO"},
	{"YES"}};

char pal60_options[2][6] = 
	{{"NO"},
	{"YES"}};

char pal50_options[2][6] = 
	{{"NO"},
	{"YES"}};

char hooktype_options[9][14] = 
	{{"No Hooks"},
	{"Default"},
	{"VBI"},
	{"Wii Pad"},
	{"GC Pad"},
	{"GXDraw"},
	{"GXFlush"},
	{"OSSleepThread"},
	{"AXNextFrame"}};

char debugger_options[2][6] = 
	{{"NO"},
	{"YES"}};

char filepatcher_options[2][6] = 
	{{"NO"},
	{"YES"}};

char ocarina_options[2][6] = 
	{{"NO"},
	{"YES"}};

char pausedstart_options[2][6] = 
	{{"NO"},
	{"YES"}};

char bubble_options[2][6] = 
	{{"NO"},
	{"YES"}};

char recovery_options[2][6] = 
	{{"NO"},
	{"YES"}};

char regionfree_options[2][6] = 
	{{"NO"},
	{"YES"}};

char nocopy_options[2][6] = 
	{{"NO"},
	{"YES"}};

char buttonskip_options[2][6] = 
	{{"NO"},
	{"YES"}};

//---------------------------------------------------------------------------------
static u32 menu_process_config_flags()
//---------------------------------------------------------------------------------
{
	if((pal60select == 1 && ntscselect == 1) || 
		(pal50select == 1 && ntscselect == 1) ||
		(pal50select == 1 && pal60select == 1)){
		error_video = 1;
		return 0;
	}
	
	switch(langselect)
	{
		case 0:
			config_bytes[0] = 0xCD;
		break;

		case 1:
			config_bytes[0] = 0x00;
		break;

		case 2:
			config_bytes[0] = 0x01;
		break;

		case 3:
			config_bytes[0] = 0x02;
		break;

		case 4:
			config_bytes[0] = 0x03;
		break;

		case 5:
			config_bytes[0] = 0x04;
		break;

		case 6:
			config_bytes[0] = 0x05;
		break;

		case 7:
			config_bytes[0] = 0x06;
		break;

		case 8:
			config_bytes[0] = 0x07;
		break;

		case 9:
			config_bytes[0] = 0x08;
		break;
		
		case 10:
			config_bytes[0] = 0x09;
		break;
	}
	
	// Video Modes
	if(pal60select == 0 && ntscselect == 0 && pal50select == 0){ // config[0] 0x00 (apply no patches)
		config_bytes[1] = 0x00;
	}

	if(pal60select == 1){			// 0x01 (Force PAL60)
		config_bytes[1] = 0x01;
	}

	if(pal50select == 1){			// 0x02 (Force PAL50)
		config_bytes[1] = 0x02;
	}

	if(ntscselect == 1){			// 0x02 (Force NTSC)
		config_bytes[1] = 0x03;
	}
	
	// Hook Type
	switch(hooktypeselect)
	{
		case 0:
			config_bytes[2] = 0x00;	// No Hooks
		break;
		
		case 1:
			config_bytes[2] = 0x08;	// Default
		break;
		
		case 2:
			config_bytes[2] = 0x01;	// VBI
		break;

		case 3:
			config_bytes[2] = 0x02;	// KPAD Read
		break;

		case 4:
			config_bytes[2] = 0x03;	// Joypad Hook
		break;

		case 5:
			config_bytes[2] = 0x04;	// GXDraw Hook
		break;

		case 6:
			config_bytes[2] = 0x05;	// GXFlush Hook
		break;

		case 7:
			config_bytes[2] = 0x06;	// OSSleepThread Hook
		break;

		case 8:
			config_bytes[2] = 0x07;	// AXNextFrame Hook
		break;
	}

	// filepatcher
	switch(filepatcherselect)
	{
		case 0:
			config_bytes[3] = 0x00;	// No file patch
		break;
		
		case 1:
			config_bytes[3] = 0x01;	// File Patch
		break;
	}

	// Ocarina
	switch(ocarinaselect)
	{
		case 0:
			config_bytes[4] = 0x00;	// No Ocarina
		break;
		
		case 1:
			config_bytes[4] = 0x01;	// Ocarina
		break;
	}

	// Gecko Paused Start
	switch(pausedstartselect)
	{
		case 0:
			config_bytes[5] = 0x00;	// No Paused Start
		break;
		
		case 1:
			config_bytes[5] = 0x01;	// Paused Start
		break;
	}

	// Gecko Port
	switch(bubbleselect)
	{
		case 0:
			config_bytes[6] = 0x00;	// No bubbles
		break;
		
		case 1:
			config_bytes[6] = 0x01;	// Bubbles
		break;
	}
	
	// Debugger
	switch(debuggerselect)
	{
		case 0:
			config_bytes[7] = 0x00;	// No Debugger
			break;
			
		case 1:
			config_bytes[7] = 0x01;	// Debugger
			break;
	}

	return 1;
}

static u32 menu_process_rebooterconf_flags()
{
	switch(recoveryselect)
	{
		case 0:
			config_bytes[8] = 0x00;
			break;
			
		case 1:
			config_bytes[8] = 0x01;
			break;
	}
	
	switch(regionfreeselect)
	{
		case 0:
			config_bytes[9] = 0x00;
			break;
			
		case 1:
			config_bytes[9] = 0x01;
			break;
	}
	
	switch(nocopyselect)
	{
		case 0:
			config_bytes[10] = 0x00;
			break;
			
		case 1:
			config_bytes[10] = 0x01;
			break;
	}
	
	switch(buttonskipselect)
	{
		case 0:
			config_bytes[11] = 0x00;
			break;
			
		case 1:
			config_bytes[11] = 0x01;
			break;
	}
	
	return 1;
}

//---------------------------------------------------------------------------------
static void menu_pad_file()
//---------------------------------------------------------------------------------
{
	
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	
	if((buttonsDown & PAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){

		if(file_menu_selected < (numfiles-1) )
		{
			file_menu_selected++;
			if(file_menu_selected >= (filetop+12) )
				filetop = file_menu_selected-11;
		}
	}

	if((buttonsDown & PAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){

		if(file_menu_selected > 0)
		{
			file_menu_selected--;
			if(file_menu_selected < filetop ){
				filetop = file_menu_selected;
			}
		}	
	}

	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		menu_number = 0;
	}

	if((buttonsDown & PAD_BUTTON_A) ||
		(WiibuttonsDown & WPAD_BUTTON_A) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){

		file_execute(filelist, filetop, 0);
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_apploader()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		if (ioswad != NULL)
		{
			free(ioswad->name);
			free(ioswad);
			ioswad = NULL;
		}
		if (app_thread_state != 12)
			menu_number = 0;
		else
			STM_RebootSystem();
	}
	
	if (ioswad != NULL &&
		((buttonsDown & PAD_BUTTON_A) ||
		(WiibuttonsDown & WPAD_BUTTON_A) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_A))){
		installios_thread();
		menu_number = 11;
	}
		
}

//---------------------------------------------------------------------------------
static void menu_pad_help()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		menu_number = 0;
	}
}


//---------------------------------------------------------------------------------
static void menu_pad_fst_extract()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		menu_number = 7;
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_installios()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
	
	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	
	if((buttonsDown & PAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		STM_RebootSystem();
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_tools()
//---------------------------------------------------------------------------------
{
	int i;
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	// Clamp and wrap if go out of bounds
	if(tools_menu_selected > tools_menu_items-1){
		tools_menu_selected = 0;
	}
	
	if(tools_menu_selected < 0){
		tools_menu_selected = tools_menu_items-1;
	}
	
	// Reset faded rows
	for(i=0;i<tools_menu_items;i++)
	{
		toolsmenu[i].selected = 0;
	}
	// Fade selected
	toolsmenu[tools_menu_selected].selected = 1;

	if((buttonsDown & PAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){
		tools_menu_selected += 1;
	}

	if((buttonsDown & PAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){
		tools_menu_selected -= 1;
	}

	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		menu_number = 0;
	}

	if((tools_menu_selected == 0 && buttonsDown & PAD_BUTTON_A) ||
		(tools_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(tools_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		sd_refresh();
	}

	if((tools_menu_selected == 1 && buttonsDown & PAD_BUTTON_A) ||
		(tools_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(tools_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		menu_number = 9;
		fst_thread();	// do thread, so menu, bubbles keep running
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_root()
//---------------------------------------------------------------------------------
{
	int i;
	s32 ret;
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	 

	// Clamp and wrap if go out of bounds
	if(root_menu_selected > root_menu_items-1){
		root_menu_selected = 0;
	}
	
	if(root_menu_selected < 0){
		root_menu_selected = root_menu_items-1;
	}
	
	// Reset faded rows
	for(i=0;i<root_menu_items;i++)
	{
		rootmenu[i].selected = 0;
	}
	// Fade selected
	rootmenu[root_menu_selected].selected = 1;

	if(buttonsDown & PAD_BUTTON_START){
		exitme();
	}
	
	if((buttonsDown & PAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_BUTTON_DOWN)	||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){
		root_menu_selected += 1;
		if (!identifyworks && root_menu_selected == 1) root_menu_selected++;
		if ((!rebooterasmenuitem || disableRebooter || !identifyworks) && root_menu_selected == 2) root_menu_selected++;
		if ((disableRebooter || !identifyworks) && root_menu_selected == 4) root_menu_selected++;
	}

	if((buttonsDown & PAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_BUTTON_UP) || 
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){
		root_menu_selected -= 1;
		if ((disableRebooter || !identifyworks) && root_menu_selected == 4) root_menu_selected--;
		if ((!rebooterasmenuitem || disableRebooter || !identifyworks) && root_menu_selected == 2) root_menu_selected--;
		if (!identifyworks && root_menu_selected == 1) root_menu_selected--;
	}

	// Root Actions
	if((root_menu_selected == 0 && buttonsDown & PAD_BUTTON_A) ||
		(root_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(root_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		menu_number = 8;
		apploader_thread();	// do thread, so menu, bubbles keep running
	}
	
	// Launch Channel
	if((root_menu_selected == 1 && buttonsDown & PAD_BUTTON_A) ||
	   (root_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_A) ||
	   (root_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		if (channellist_menu_items == 0)
		{
			if (!menu_generatechannellist(FALSE))
			{
				menu_number = 14;
				channel_thread_state = 7;
			}
			else
				menu_number = 15;
		}
		else
			menu_number = 15;
	}
	
	// Launch Rebooter
	if((root_menu_selected == 2 && buttonsDown & PAD_BUTTON_A) ||
	   (root_menu_selected == 2 && WiibuttonsDown & WPAD_BUTTON_A) ||
	   (root_menu_selected == 2 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		rebooter_thread();
		menu_number = 12;
		rebooter_thread_state = 0;
	}

	// Switch to Config Menu
	if((root_menu_selected == 3 && buttonsDown & PAD_BUTTON_A) ||
		(root_menu_selected == 3 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(root_menu_selected == 3 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		menu_number = 1;
	}
	
	// Switch to Rebooter Config Menu
	if((root_menu_selected == 4 && buttonsDown & PAD_BUTTON_A) ||
	   (root_menu_selected == 4 && WiibuttonsDown & WPAD_BUTTON_A) ||
	   (root_menu_selected == 4 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		menu_number = 13;
	}

		// About
	if((root_menu_selected == 5 && buttonsDown & PAD_BUTTON_A) ||
		(root_menu_selected == 5 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(root_menu_selected == 5 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		menu_number = 10;
	}

	// Exit
	if((root_menu_selected == 6 && buttonsDown & PAD_BUTTON_A) ||
		(root_menu_selected == 6 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(root_menu_selected == 6 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		exitme();
	}
	
	if (!rebooterasmenuitem && !disableRebooter && identifyworks)
	{
		if((buttonsDown & PAD_BUTTON_B) ||
		   (WiibuttonsDown & WPAD_BUTTON_B) ||
		   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
			rebooter_thread();
			menu_number = 12;
			rebooter_thread_state = 0;
		}
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_config()
//---------------------------------------------------------------------------------
{
	int i;
	u32 ret;
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);

	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);

	// Clamp and wrap if go out of bounds
	if(config_menu_selected > config_menu_items-1){
		config_menu_selected = 0;
	}
	
	if(config_menu_selected < 0){
		config_menu_selected = config_menu_items-1;
	}
	// Reset faded rows
	for(i=0;i<config_menu_items;i++)
	{
		configmenu[i].selected = 0;
	}
	if(config_menu_selected < 0){
		config_menu_selected = config_menu_items-1;
	}
	// Fade selected
	configmenu[config_menu_selected].selected = 1;

	if((buttonsDown & PAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_BUTTON_DOWN) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){
		config_menu_selected += 1;
	}

	if((buttonsDown & PAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_BUTTON_UP) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){
		config_menu_selected -= 1;
	}

	// Config Actions
	// Language
	if((config_menu_selected == 0 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(langselect == 0){
				langselect = 0;
			}
			else {
				langselect -= 1;
			}	
	}
	if((config_menu_selected == 0 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(langselect == 10){
				langselect = 10;
			}
			else {
				langselect += 1;
			}		
	}

	// Force NTSC
	if((config_menu_selected == 1 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(ntscselect == 0){
			ntscselect = 0;
		}
		else {
			ntscselect -= 1;
		}	
	}

	if((config_menu_selected == 1 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(ntscselect == 1){
			ntscselect = 1;
		}
		else {
			ntscselect += 1;
		}	
	}

	// Force PAL60
	if((config_menu_selected == 2 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 2 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 2 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(pal60select == 0){
			pal60select = 0;
		}
		else {
			pal60select -= 1;
		}	
	}

	if((config_menu_selected == 2 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 2 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 2 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(pal60select == 1){
			pal60select = 1;
		}
		else {
			pal60select += 1;
		}	
	}

	// Force PAL50
	if((config_menu_selected == 3 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 3 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 3 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(pal50select == 0){
			pal50select = 0;
		}
		else {
			pal50select -= 1;
		}	
	}

	if((config_menu_selected == 3 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 3 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 3 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(pal50select == 1){
			pal50select = 1;
		}
		else {
			pal50select += 1;
		}	
	}

	// Hook Type
	if((config_menu_selected == 4 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 4 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 4 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(hooktypeselect == 0){
			hooktypeselect = 0;
		}
		else {
			hooktypeselect -= 1;
		}	
	}
	if((config_menu_selected == 4 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 4 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 4 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(hooktypeselect == 8){
			hooktypeselect = 8;
		}
		else {
			hooktypeselect += 1;
		}	
	}
	
	if((config_menu_selected == 5 && buttonsDown & PAD_BUTTON_LEFT) ||
	   (config_menu_selected == 5 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
	   (config_menu_selected == 5 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(debuggerselect == 0){
			debuggerselect = 0;
		}
		else {
			debuggerselect -= 1;
		}	
	}
	
	if((config_menu_selected == 5 && buttonsDown & PAD_BUTTON_RIGHT) ||
	   (config_menu_selected == 5 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
	   (config_menu_selected == 5 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(debuggerselect == 1){
			debuggerselect = 1;
		}
		else {
			debuggerselect += 1;
		}	
	}

	// File Patcher on
	if((config_menu_selected == 6 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 6 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 6 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(filepatcherselect == 0){
			filepatcherselect = 0;
		}
		else {
			filepatcherselect -= 1;
		}	
	}

	if((config_menu_selected == 6 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 6 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 6 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(filepatcherselect == 1){
			filepatcherselect = 1;
		}
		else {
			filepatcherselect += 1;
		}	
	}
	// Ocarina on
	if((config_menu_selected == 7 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 7 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 7 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(ocarinaselect == 0){
			ocarinaselect = 0;
		}
		else {
			ocarinaselect -= 1;
		}	
	}

	if((config_menu_selected == 7 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 7 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 7 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(ocarinaselect == 1){
			ocarinaselect = 1;
		}
		else {
			ocarinaselect += 1;
		}	
	}

	// Paused Start On
	if((config_menu_selected == 8 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 8 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 8 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(pausedstartselect == 0){
			pausedstartselect = 0;
		}
		else {
			pausedstartselect -= 1;
		}	
	}

	if((config_menu_selected == 8 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 8 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 8 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT) ){
		if(pausedstartselect == 1){
			pausedstartselect = 1;
		}
		else {
			pausedstartselect += 1;
		}	
	}
	// Paused Start On
	if((config_menu_selected == 9 && buttonsDown & PAD_BUTTON_LEFT) ||
		(config_menu_selected == 9 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
		(config_menu_selected == 9 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(bubbleselect == 0){
			bubbleselect = 0;
		}
		else {
			bubbleselect -= 1;
		}	
	}

	if((config_menu_selected == 9 && buttonsDown & PAD_BUTTON_RIGHT) ||
		(config_menu_selected == 9 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
		(config_menu_selected == 9 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(bubbleselect == 1){
			bubbleselect = 1;
		}
		else {
			bubbleselect += 1;
		}	
	}

	// Save config
	if((config_menu_selected == 10 && buttonsDown & PAD_BUTTON_A) ||
		(config_menu_selected == 10 && WiibuttonsDown & WPAD_BUTTON_A) ||
		(config_menu_selected == 10 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		ret = menu_process_config_flags();	// save the config bytes in mem
		if(ret){
			if(sd_found == 1){
				sd_save_config();
				confirm_sd = 1;
			}
			else {
				error_sd = 1;
			}
		}		
	}
	// Return to Main Menu
	if((buttonsDown & PAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_BUTTON_B) ||
		(WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		ret = menu_process_config_flags();
		if(ret){	
			menu_number = 0;
		}
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_rebooterconf()
//---------------------------------------------------------------------------------
{
	int i;
	u32 ret;
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
	
	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	
	// Clamp and wrap if go out of bounds
	if(rebooterconf_menu_selected > rebooterconf_menu_items-1){
		rebooterconf_menu_selected = 0;
	}
	
	if(rebooterconf_menu_selected < 0){
		rebooterconf_menu_selected = rebooterconf_menu_items-1;
	}
	// Reset faded rows
	for(i=0;i<rebooterconf_menu_items;i++)
	{
		rebooterconfmenu[i].selected = 0;
	}
	if(rebooterconf_menu_selected < 0){
		rebooterconf_menu_selected = rebooterconf_menu_items-1;
	}
	// Fade selected
	rebooterconfmenu[rebooterconf_menu_selected].selected = 1;
	
	if((buttonsDown & PAD_BUTTON_DOWN) ||
	   (WiibuttonsDown & WPAD_BUTTON_DOWN) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){
		rebooterconf_menu_selected += 1;
	}
	
	if((buttonsDown & PAD_BUTTON_UP) ||
	   (WiibuttonsDown & WPAD_BUTTON_UP) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){
		rebooterconf_menu_selected -= 1;
	}
	
	// Config Actions
	
	if((rebooterconf_menu_selected == 0 && buttonsDown & PAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(recoveryselect == 0){
			recoveryselect = 0;
		}
		else {
			recoveryselect -= 1;
		}	
	}
	
	if((rebooterconf_menu_selected == 0 && buttonsDown & PAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 0 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 0 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(recoveryselect == 1){
			recoveryselect = 1;
		}
		else {
			recoveryselect += 1;
		}	
	}
	
	if((rebooterconf_menu_selected == 1 && buttonsDown & PAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(regionfreeselect == 0){
			regionfreeselect = 0;
		}
		else {
			regionfreeselect -= 1;
		}	
	}
	
	if((rebooterconf_menu_selected == 1 && buttonsDown & PAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 1 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 1 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(regionfreeselect == 1){
			regionfreeselect = 1;
		}
		else {
			regionfreeselect += 1;
		}	
	}
	
	if((rebooterconf_menu_selected == 2 && buttonsDown & PAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 2 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 2 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(nocopyselect == 0){
			nocopyselect = 0;
		}
		else {
			nocopyselect -= 1;
		}	
	}
	
	if((rebooterconf_menu_selected == 2 && buttonsDown & PAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 2 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 2 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(nocopyselect == 1){
			nocopyselect = 1;
		}
		else {
			nocopyselect += 1;
		}	
	}
	
	// Hook Type
	if((rebooterconf_menu_selected == 3 && buttonsDown & PAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 3 && WiibuttonsDown & WPAD_BUTTON_LEFT) ||
	   (rebooterconf_menu_selected == 3 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_LEFT)){
		if(buttonskipselect == 0){
			buttonskipselect = 0;
		}
		else {
			buttonskipselect -= 1;
		}	
	}
	if((rebooterconf_menu_selected == 3 && buttonsDown & PAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 3 && WiibuttonsDown & WPAD_BUTTON_RIGHT) ||
	   (rebooterconf_menu_selected == 3 && WiibuttonsDown & WPAD_CLASSIC_BUTTON_RIGHT)){
		if(buttonskipselect == 1){
			buttonskipselect = 1;
		}
		else {
			buttonskipselect += 1;
		}	
	}
	
	// Return to Main Menu
	if((buttonsDown & PAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		ret = menu_process_rebooterconf_flags();
		if(ret){	
			menu_number = 0;
		}
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_rebooter()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
	
	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	
	if((buttonsDown & PAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		rebooter_thread_close();
		autoboot = 0;
		menu_number = 0;
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_channellist()
//---------------------------------------------------------------------------------
{
	int i;
	u32 ret;
	
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
	
	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	
	// Reset faded rows
	for(i=0;i<channellist_menu_items;i++)
	{
		channellistmenu[i].selected = 0;
	}
	if(channellist_menu_selected < 0){
		channellist_menu_selected = channellist_menu_items-1;
	}
	// Fade selected
	channellistmenu[channellist_menu_selected].selected = 1;
	
	if((buttonsDown & PAD_BUTTON_DOWN) ||
	   (WiibuttonsDown & WPAD_BUTTON_DOWN) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_DOWN)){
		channellist_menu_selected += 1;
		if (channellist_menu_selected >= channellist_menu_items)
		{
			channellist_menu_selected = 0;
			channellist_menu_top = 0;
		}
		else if (channellist_menu_selected - channellist_menu_top > 10)
			channellist_menu_top++;
	}
	
	if((buttonsDown & PAD_BUTTON_UP) ||
	   (WiibuttonsDown & WPAD_BUTTON_UP) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_UP)){
		channellist_menu_selected -= 1;
		if (channellist_menu_selected < 0)
		{
			channellist_menu_selected = channellist_menu_items-1;
			channellist_menu_top = (channellist_menu_items > 11) ? channellist_menu_items-11 : 0;
		}
		else if (channellist_menu_top > channellist_menu_selected)
			channellist_menu_top--;
	}
	
	if((buttonsDown & PAD_BUTTON_A) ||
	   (WiibuttonsDown & WPAD_BUTTON_A) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_A)){
		channeltoload = channellistmenu[channellist_menu_selected].titleid;
		channel_thread();
		menu_number = 14;
	}
	
	if((buttonsDown & PAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		menu_number = 0;
	}
}

//---------------------------------------------------------------------------------
static void menu_pad_bootchannel()
//---------------------------------------------------------------------------------
{
	PAD_ScanPads();
    u32 buttonsDown = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
	
	WPAD_ScanPads();
	u32 WiibuttonsDown = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
	
	if((buttonsDown & PAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_BUTTON_B) ||
	   (WiibuttonsDown & WPAD_CLASSIC_BUTTON_B)){
		if (channel_thread_state == 1)
			STM_RebootSystem();
		else
			menu_number = 0;
	}
}

//---------------------------------------------------------------------------------
static void menu_drawroot()
//---------------------------------------------------------------------------------
{
	int i;

	menu_pad_root();

	for(i=0;i<root_menu_items;i++)
	{
		if (((rebooterasmenuitem && !disableRebooter && identifyworks) || i != 2) && (identifyworks || i != 1) && ((!disableRebooter && identifyworks) || i != 4))
		{
			if(rootmenu[i].selected == 1){
				gfx_print(tex_font1, font_x_screen, font_y_screen, textfade,rootmenu[i].ritems);
			}
			else {
				gfx_print(tex_font1, font_x_screen, font_y_screen, 0xff,rootmenu[i].ritems);
			}
			font_y_screen += font_y_spacing;
		}
	}
	font_y_screen += font_y_spacing;
	
	if (!rebooterasmenuitem && !disableRebooter && identifyworks)
	{
		gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Press B to Launch Rebooter");
		font_y_screen += font_y_spacing;
	}
	
	if (iosversion != 36)
		gfx_printf(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Using IOS %d", iosversion);

	font_y_screen = font_y_pos;
}

//---------------------------------------------------------------------------------
void menu_load_config()
//---------------------------------------------------------------------------------
{
	// load the bytes from SD
	switch(config_bytes[0])
	{
		case 0xCD:
			langselect  = 0;
		break;

		case 0x00:
			langselect  = 1;
		break;

		case 0x01:
			langselect  = 2;
		break;

		case 0x02:
			langselect  = 3;
		break;

		case 0x03:
			langselect  = 4;
		break;

		case 0x04:
			langselect  = 5;
		break;

		case 0x05:
			langselect  = 6;
		break;
		
		case 0x06:
			langselect  = 7;
		break;
		
		case 0x07:
			langselect  = 8;
		break;
		
		case 0x08:
			langselect  = 9;
		break;
		
		case 0x09:
			langselect  = 10;
		break;
	}

	switch(config_bytes[1])
	{
		case 0x00:	// apply no patches, disabled
			pal60select = 0;
			ntscselect = 0;
			pal50select = 0;
		break;

		case 0x01:	// 0x01 (Force PAL60)
			pal60select = 1;
		break;

		case 0x02:	// 0x02 (Force PAL50)
			pal50select = 1;
		break;

		case 0x03:	// 0x03 (Force NTSC)
			ntscselect = 1;
		break;
	}

	// Hook Type
	switch(config_bytes[2])
	{
		case 0x00:
			hooktypeselect = 0;	// No Hooks
		break;
		
		case 0x01:
			hooktypeselect = 2;	// VBI
		break;

		case 0x02:
			hooktypeselect = 3;	// KPAD Read
		break;

		case 0x03:
			hooktypeselect = 4;	// Joypad Hook
		break;
			
		case 0x04:
			hooktypeselect = 5;	// GXDraw Hook
		break;
			
		case 0x05:
			hooktypeselect = 6;	// GXFlush Hook
		break;
			
		case 0x06:
			hooktypeselect = 7;	// OSSleepThread Hook
		break;
			
		case 0x07:
			hooktypeselect = 8;	// AXNextFrame Hook
		break;
			
		case 0x08:
			hooktypeselect = 1;	// Default
		break;
	}
	
	// filepatcher
	switch(config_bytes[3])
	{
		case 0x00:
			filepatcherselect = 0;	// No file patch
		break;
		
		case 0x01:
			filepatcherselect = 1;	// File Patch
		break;
	}
	
	// Ocarina
	switch(config_bytes[4])
	{
		case 0x00:
			ocarinaselect = 0;	// No Ocarina
		break;
		
		case 0x01:
			ocarinaselect = 1;	// Ocarina
		break;
	}

	// Paused Start
	switch(config_bytes[5])
	{
		case 0x00:
			pausedstartselect = 0;	// No Paused Start
		break;
		
		case 0x01:
			pausedstartselect = 1;	// Paused Start
		break;
	}

	// Gecko Slot
	switch(config_bytes[6])
	{
		case 0x00:
			bubbleselect = 0;	// Slot B
		break;
		
		case 0x01:
			bubbleselect = 1;	// Slot A
		break;
	}
	
	// Debugger
	switch(config_bytes[7])
	{
		case 0x00:
			debuggerselect = 0;	// No Debugger
			break;
			
		case 0x01:
			debuggerselect = 1;	// Debugger
			break;
	}
	
	switch(config_bytes[8])
	{
		case 0x00:
			recoveryselect = 0;
			break;
			
		case 0x01:
			recoveryselect = 1;
			break;
	}
	
	switch(config_bytes[9])
	{
		case 0x00:
			regionfreeselect = 0;
			break;
			
		case 0x01:
			regionfreeselect = 1;
			break;
	}
	
	switch(config_bytes[10])
	{
		case 0x00:
			nocopyselect = 0;
			break;
			
		case 0x01:
			nocopyselect = 1;
			break;
	}
	
	switch(config_bytes[11])
	{
		case 0x00:
			buttonskipselect = 0;
			break;
			
		case 0x01:
			buttonskipselect = 1;
			break;
	}
}


//---------------------------------------------------------------------------------
static void menu_drawconfig()
//---------------------------------------------------------------------------------
{
	int i;

	menu_pad_config();
	
	for(i=0;i<config_menu_items;i++)
	{
		if(configmenu[i].selected == 1){
			gfx_print(tex_font1, config_font_x_pos, font_y_screen, textfade,configmenu[i].citems);
		}
		else {
			gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,configmenu[i].citems);
		}
		font_y_screen += font_y_spacing;
	}
		font_y_screen += font_y_spacing;
		gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Press B to return to menu");

		if(error_video){
			font_y_screen += font_y_spacing;
			gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Can't force two video modes");
		}

		if(error_sd){
			font_y_screen += font_y_spacing;
			gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"No SD card found");
		}

		if(confirm_sd){
			font_y_screen += font_y_spacing;
			gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Config saved");
		}

	font_y_screen = font_y_pos;

	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen, 0xff,languages[langselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+22, 0xff,ntsc_options[ntscselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+44, 0xff,pal60_options[pal60select]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+66, 0xff,pal50_options[pal50select]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+88, 0xff,hooktype_options[hooktypeselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+110, 0xff,debugger_options[debuggerselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+132, 0xff,filepatcher_options[filepatcherselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+154, 0xff,ocarina_options[ocarinaselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+176, 0xff,pausedstart_options[pausedstartselect]);
	gfx_print(tex_font1, config_font_x_pos+190, font_y_screen+198, 0xff,bubble_options[bubbleselect]);
	
	// load values from SD card, or default. so preloads values
	if(config_not_loaded == 0){
		menu_load_config();
		config_not_loaded = 1;
	}

	font_y_screen = font_y_pos;
}

//---------------------------------------------------------------------------------
static void menu_drawrebooterconf()
//---------------------------------------------------------------------------------
{
	int i;
	
	menu_pad_rebooterconf();
	
	for(i=0;i<rebooterconf_menu_items;i++)
	{
		if(rebooterconfmenu[i].selected == 1){
			gfx_print(tex_font1, rebooterconf_font_x_pos, font_y_screen, textfade,rebooterconfmenu[i].rcitems);
		}
		else {
			gfx_print(tex_font1, rebooterconf_font_x_pos, font_y_screen, 0xff,rebooterconfmenu[i].rcitems);
		}
		font_y_screen += font_y_spacing;
	}
	font_y_screen += font_y_spacing;
	gfx_print(tex_font1, rebooterconf_font_x_pos, font_y_screen, 0xff,"Press B to return to menu");
	
	font_y_screen = font_y_pos;
	
	gfx_print(tex_font1, rebooterconf_font_x_pos+190, font_y_screen, 0xff,recovery_options[recoveryselect]);
	gfx_print(tex_font1, rebooterconf_font_x_pos+190, font_y_screen+22, 0xff,regionfree_options[regionfreeselect]);
	gfx_print(tex_font1, rebooterconf_font_x_pos+190, font_y_screen+44, 0xff,nocopy_options[nocopyselect]);
	gfx_print(tex_font1, rebooterconf_font_x_pos+190, font_y_screen+66, 0xff,buttonskip_options[buttonskipselect]);
	
	// load values from SD card, or default. so preloads values
	if(config_not_loaded == 0){
		menu_load_config();
		config_not_loaded = 1;
	}
	
	font_y_screen = font_y_pos;
}

//---------------------------------------------------------------------------------
static void menu_drawhelp()
//---------------------------------------------------------------------------------
{

	int i;

	menu_pad_help();

	for(i=0;i<help_menu_items;i++)
	{
		if(helpmenu[i].selected == 1){
			gfx_print(tex_font1, tools_font_x_pos, font_y_screen, textfade,helpmenu[i].hitems);
		}
		else{
			gfx_print(tex_font1, tools_font_x_pos, font_y_screen, 0xff,helpmenu[i].hitems);
		}
		font_y_screen += font_y_spacing;
	}

	font_y_screen += font_y_spacing;
	
	switch(gecko_channel)
		{
			case 0:
				gfx_print(tex_font1,tools_font_x_pos, font_y_screen, 0xff,"USB Gecko in Slot A");
			break;

			case 1:
				gfx_print(tex_font1,tools_font_x_pos, font_y_screen, 0xff,"USB Gecko in Slot B");
			break;

			case 2:
				gfx_print(tex_font1,tools_font_x_pos, font_y_screen, 0xff,"USB Gecko not found");
			break;

		}
	font_y_screen += 44;
	gfx_print(tex_font1, tools_font_x_pos, font_y_screen, 0xff,"Press B to return to menu");

	font_y_screen = font_y_pos;
}

//---------------------------------------------------------------------------------
static void menu_drawtools()
//---------------------------------------------------------------------------------
{

	int i;

	menu_pad_tools();

	for(i=0;i<tools_menu_items;i++)
	{
		if(toolsmenu[i].selected == 1){
			gfx_print(tex_font1, tools_font_x_pos, font_y_screen, textfade,toolsmenu[i].titems);
		}
		else{
			gfx_print(tex_font1, tools_font_x_pos, font_y_screen, 0xff,toolsmenu[i].titems);
		}
		font_y_screen += font_y_spacing;
	}
	font_y_screen += font_y_spacing;
	gfx_print(tex_font1, tools_font_x_pos, font_y_screen, 0xff,"Press B to return to menu");

	font_y_screen = font_y_pos;
}


//---------------------------------------------------------------------------------
static void menu_drawcheat(){
//---------------------------------------------------------------------------------

}

//---------------------------------------------------------------------------------
static void menu_drawextract()
//---------------------------------------------------------------------------------
{
	switch (fst_thread_state)
	{
		case 1:
			menu_pad_fst_extract();
			gfx_print(tex_font1,font_x_screen-20, font_y_screen, 0xff,"Error: No SD Card Found");
			gfx_print(tex_font1,font_x_screen-20, font_y_screen+44, 0xff,"Press B to Return");
			fst_thread_close();		// close thread
		break;

		case 2:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
		break;

		case 3:
			menu_pad_fst_extract();
			gfx_print(tex_font1,font_x_screen-20, font_y_screen, 0xff,"Error: No DVD Found");
			gfx_print(tex_font1,font_x_screen-20, font_y_screen+44, 0xff,"Press B to Return");
			fst_thread_close();		// close thread
		break;

		case 4:
			menu_pad_fst_extract();
			gfx_print(tex_font1,font_x_screen-20, font_y_screen, 0xff,"Error: Gamecube DVD");
			gfx_print(tex_font1,font_x_screen-20, font_y_screen+44, 0xff,"Press B to Return");
			fst_thread_close();		// close thread
		break;

		case 5:
			gfx_print(tex_font1,font_x_screen-40, font_y_screen, 0xff,"Extracting Files, Please Wait");
		break;

		case 6:
			gfx_print(tex_font1,font_x_screen-55, font_y_screen, 0xff,"Extracting Files, Please Wait");
			if(no_entries >= real_entries){
				gfx_printf(tex_font1,font_x_screen-55, font_y_screen+22, 0xff,"Extracting file %d of %d to SD (wad)\n",real_entries,real_entries);
				break;
			}
			gfx_printf(tex_font1,font_x_screen-55, font_y_screen+22, 0xff,"Extracting file %d of %d to SD (wad)\n",no_entries,real_entries);
		break;

		case 7:
			menu_pad_fst_extract();
			gfx_print(tex_font1,font_x_screen-20, font_y_screen, 0xff,"Error: File Error");
			gfx_print(tex_font1,font_x_screen-20, font_y_screen+44, 0xff,"Press B to Return");
			fst_thread_close();		// close thread
		break;

		case 8:
			menu_pad_fst_extract();
			gfx_print(tex_font1,font_x_screen-55, font_y_screen, 0xff,"Extracting Files, Please Wait");
			gfx_printf(tex_font1,font_x_screen-55, font_y_screen+22, 0xff,"Extracting file %d of %d to SD (wad)\n",real_entries,real_entries);
			gfx_print(tex_font1,font_x_screen-55, font_y_screen+44, 0xff,"Extraction Complete");
			gfx_print(tex_font1,font_x_screen-55, font_y_screen+88, 0xff,"Press B to Return");
			fst_thread_close();		// close thread
		break;
	}

}

//---------------------------------------------------------------------------------
static void menu_drawapp()
//---------------------------------------------------------------------------------
{
	switch (app_thread_state)
	{
		case 1:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
		break;

		case 2:
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Error: No DVD Found");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			apploader_thread_close(); // close thread
		break;

		case 3:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+22,0xff,"Reading DVD Stats..");
		break;

		case 4:
			gfx_printf(tex_font1,font_x_screen-45,font_y_screen,0xff,"This Game Requires IOS %d",dvdios);
			if (ioswad != NULL)
			{
				gfx_printf(tex_font1,font_x_screen-45,font_y_screen+22,0xff,"Install %s?",ioswad->name);
				gfx_print(tex_font1,font_x_screen-45, font_y_screen+66, 0xff,"Press A to Install or B to Cancel");
			}
			else
			{
				gfx_print(tex_font1,font_x_screen-45,font_y_screen+22,0xff,"IOS Not Found on Disc");
				gfx_print(tex_font1,font_x_screen-45, font_y_screen+66, 0xff,"Press B to Return");
			}
			WPAD_Init();
			menu_pad_apploader();
			apploader_thread_close(); // close thread
		break;

		case 5:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+22,0xff,"Reading DVD Stats..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+44,0xff,"Loading Title..");
		break;	

		case 6:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+22,0xff,"Reading DVD Stats..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+44,0xff,"Loading Title..");
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
		break;

		case 8:		
			apploader_thread_close(); // close thread
			VIDEO_SetBlack(TRUE);
			VIDEO_Flush();
			//sdio_Deinitialize();
			SYS_ResetSystem(SYS_SHUTDOWN,0,0);
			if (config_bytes[2] == 0)
			{
				__asm__(
						"lis %r3, appentrypoint@h\n"
						"ori %r3, %r3, appentrypoint@l\n"
						"lwz %r3, 0(%r3)\n"
						"mtlr %r3\n"
						"blr\n"
						);
			}
			else
			{
				__asm__(
						"lis %r3, appentrypoint@h\n"
						"ori %r3, %r3, appentrypoint@l\n"
						"lwz %r3, 0(%r3)\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						);
			}
		break;

		case 9:
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"SD Patch File Not Found");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			apploader_thread_close(); // close thread
		break;

		case 10:
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"SD Patch File Error");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			apploader_thread_close(); // close thread
		break;

		case 11:
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"File Read Error");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			apploader_thread_close(); // close thread
		break;
			
		case 12:
			WPAD_Init();
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Setting Up Drive..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+22,0xff,"Reading DVD Stats..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+44,0xff,"Loading Title..");
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Disc Read Error");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+110, 0xff,"Press B to Restart");
			apploader_thread_close(); // close thread
		break;
			
		case 13:
			WPAD_Init();
			menu_pad_apploader();
			gfx_print(tex_font1,font_x_screen-5,font_y_screen,0xff,"Not A Wii or Gamecube Game");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			apploader_thread_close(); // close thread
		break;
	}
	switch (codes_state)
	{
		case 0:
			break;
		case 1:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"No SD Codes Found");
			break;
		case 2:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"SD Codes Found. Applying");
			break;
		case 3:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"No Hook, Not Applying Codes");
			break;
		case 4:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"Codes Error: Too Many Codes");
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+132,0xff,"%u Lines Found, %u Lines Allowed", (codelistsize / 8) - 2, ((codelistend - codelist) / 8) - 3);
			break;
	}
	if (configwarn != 0)
		gfx_print(tex_font1,font_x_screen-5,font_y_screen+154,0xff,"Settings Overridden By Game Config:");
	switch (configwarn)
	{
		case 0:
			break;
		case 1:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Video Mode");
			break;
		case 2:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Language");
			break;
		case 3:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Video Mode, Language");
			break;
		case 4:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Hook");
			break;
		case 5:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Video Mode, Hook");
			break;
		case 6:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Language, Hook");
			break;
		case 7:
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+176,0xff,"Video Mode, Language, Hook");
			break;
	}
}

//---------------------------------------------------------------------------------
static void menu_drawinstallios()
//---------------------------------------------------------------------------------
{
	switch (installios_thread_state)
	{
		case 0:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Extracting Files From Wad...");
			break;
		case 1:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Installing Files From Wad...");
			gfx_printf(tex_font1,font_x_screen-5, font_y_screen+22, 0xff,"%u of %u", currentfile, filecount);
			break;
		case 2:
			menu_pad_installios();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Installing Files From Wad...");
			if (iosinstallret == 0)
				gfx_print(tex_font1,font_x_screen-5, font_y_screen+22, 0xff,"Done!");
			else if (iosinstallret == 5)
			{
				gfx_print(tex_font1,font_x_screen-5, font_y_screen+22, 0xff,"Error: Wad Misnamed");
				gfx_printf(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Title In Wad: %016llx", iosinstalltitle);
				gfx_printf(tex_font1,font_x_screen-5, font_y_screen+66, 0xff,"Title Expected: %016llx", 0x0000000100000000ULL | dvdios);
				gfx_print(tex_font1,font_x_screen-5, font_y_screen+110, 0xff,"Press B to Restart");
			}
			else
				gfx_printf(tex_font1,font_x_screen-5, font_y_screen+22, 0xff,"Error: Return Value %d", iosinstallret);
			if (iosinstallret != 5)
				gfx_print(tex_font1,font_x_screen-5, font_y_screen+66, 0xff,"Press B to Restart");
			installios_thread_close();
			break;
			
	}
}

//---------------------------------------------------------------------------------
static void menu_drawrebooter()
//---------------------------------------------------------------------------------
{
	switch (rebooter_thread_state)
	{
		case 0:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Rebooting With Hooks");
			if (autoboot == 1 && autobootwait > 0)
			{
				gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B For Gecko Menu");
				menu_pad_rebooter();
			}
			break;
		case 1:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Error Loading Menu");
			break;
		case 2:
			rebooter_thread_close();
			
			boot_menu();
			//sdio_Shutdown();
			SYS_ResetSystem(SYS_SHUTDOWN,0,0);
			if (config_bytes[2] != 0x00)
			{
				// Load Sysmenu patch handler
				memset((void*)0x81700000,0,sysmenu_size);
				memcpy((void*)0x81700000,sysmenu,sysmenu_size);
				memcpy((void*)0x81700096, &codelist, 2);
				memcpy((void*)0x8170009A, ((u8*) &codelist) + 2, 2);
				switch(config_bytes[2])
				{
					case 0x01:
						memcpy((void*)0x81700224,viwiihooks,12);
						memcpy((void*)0x81700220,viwiihooks+3,4);
						break;
					case 0x02:
						memcpy((void*)0x81700224,kpadhooks,12);
						memcpy((void*)0x81700220,kpadhooks+3,4);
						break;
					case 0x03:
						memcpy((void*)0x81700224,joypadhooks,12);
						memcpy((void*)0x81700220,joypadhooks+3,4);
						break;
					case 0x04:
						memcpy((void*)0x81700224,gxdrawhooks,12);
						memcpy((void*)0x81700220,gxdrawhooks+3,4);
						break;
					case 0x05:
						memcpy((void*)0x81700224,gxflushhooks,12);
						memcpy((void*)0x81700220,gxflushhooks+3,4);
						break;
					case 0x06:
						memcpy((void*)0x81700224,ossleepthreadhooks,12);
						memcpy((void*)0x81700220,ossleepthreadhooks+3,4);
						break;
					case 0x07:
						memcpy((void*)0x81700224,axnextframehooks,12);
						memcpy((void*)0x81700220,axnextframehooks+3,4);
						break;
					case 0x08:
						if (customhooksize == 16)
						{
							memcpy((void*)0x81700224,customhook,12);
							memcpy((void*)0x81700220,customhook+3,4);
						}
						break;
				}
				DCFlushRange((void*)0x81700000,sysmenu_size);
			}
			if (config_bytes[2] == 0)
			{
				__asm__(
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, 0x0000\n"
						"ori %r4,%r4,0x3400\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
			}
			else
			{
				__asm__(
						"lis %r3, returnpointmenu@h\n"
						"ori %r3, %r3, returnpointmenu@l\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						"returnpointmenu:\n"
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, 0x0000\n"
						"ori %r4,%r4,0x3400\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
			}
			break;
		case 3:
			rebooter_thread_close();
			menu_number = 0;
			break;
	}
	switch (codes_state)
	{
		case 0:
			break;
		case 1:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"No SD Codes Found");
			break;
		case 2:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"SD Codes Found. Applying");
			break;
		case 3:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"No Hook, Not Applying Codes");
			break;
		case 4:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Codes Error: Too Many Codes");
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"%u Lines Found, %u Lines Allowed", (codelistsize / 8) - 2, ((codelistend - codelist) / 8) - 2);
			break;
		case 5:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+66, 0xff,"No DVD");
			break;
		case 6:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Game ID: %s", gameidbuffer);
			break;
	}
}

//---------------------------------------------------------------------------------
static void menu_drawbootchannel()
//---------------------------------------------------------------------------------
{
	s32 ret;
	
	switch (channel_thread_state)
	{
		case 0:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Booting Channel With Hooks");
			break;
		case 1:
			menu_pad_bootchannel();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Error Loading Channel");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Restart");
			break;
		case 2:
			channel_thread_close();
			//sdio_Deinitialize();
			
			boot_channel();
			SYS_ResetSystem(SYS_SHUTDOWN,0,0);
			if (config_bytes[2] == 0)
			{
				__asm__(
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, bootentrypoint@h\n"
						"ori %r4,%r4,bootentrypoint@l\n"
						"lwz %r4, 0(%r4)\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
			}
			else
			{	
				__asm__(
						"lis %r3, returnpoint@h\n"
						"ori %r3, %r3, returnpoint@l\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						"returnpoint:\n"
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, bootentrypoint@h\n"
						"ori %r4,%r4,bootentrypoint@l\n"
						"lwz %r4, 0(%r4)\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
			}
			break;
		case 3:
			channel_thread_close();
			menu_number = 0;
			break;
		case 4:
			menu_pad_bootchannel();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"SD Patch File Not Found");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			channel_thread_close(); // close thread
			break;
			
		case 5:
			menu_pad_bootchannel();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"SD Patch File Error");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			channel_thread_close(); // close thread
			break;
			
		case 6:
			menu_pad_bootchannel();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"File Read Error");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			channel_thread_close(); // close thread
			break;
			
		case 7:
			menu_pad_bootchannel();
			gfx_print(tex_font1,font_x_screen-5, font_y_screen, 0xff,"Error Getting Channel List");
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+44, 0xff,"Press B to Return");
			break;
	}
	switch (codes_state)
	{
		case 0:
			break;
		case 1:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Channel ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"No SD Codes Found");
			break;
		case 2:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Channel ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"SD Codes Found. Applying");
			break;
		case 3:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Channel ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"No Hook, Not Applying Codes");
			break;
		case 4:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Channel ID: %s", gameidbuffer);
			gfx_print(tex_font1,font_x_screen-5,font_y_screen+88,0xff,"Codes Error: Too Many Codes");
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+110,0xff,"%u Lines Found, %u Lines Allowed", (codelistsize / 8) - 2, ((codelistend - codelist) / 8) - 2);
			break;
		case 5:
			gfx_print(tex_font1,font_x_screen-5, font_y_screen+66, 0xff,"No Channel Inserted");
			break;
		case 6:
			gfx_printf(tex_font1,font_x_screen-5,font_y_screen+66,0xff,"Channel ID: %s", gameidbuffer);
			break;
	}
}

//---------------------------------------------------------------------------------
static void menu_drawchannellist()
//---------------------------------------------------------------------------------
{
	int i;
	
	menu_pad_channellist();
	
	for(i=channellist_menu_top;i<((channellist_menu_top+11 > channellist_menu_items) ? channellist_menu_items : channellist_menu_top+11);i++)
	{
		if(channellistmenu[i].selected == 1){
			gfx_print(tex_font1, font_x_screen-180, font_y_screen, textfade,channellistmenu[i].clitems);
		}
		else {
			gfx_print(tex_font1, font_x_screen-180, font_y_screen, 0xff,channellistmenu[i].clitems);
		}
		font_y_screen += font_y_spacing;
	}
	font_y_screen += font_y_spacing;
	
	gfx_print(tex_font1, config_font_x_pos, font_y_screen, 0xff,"Press B to Return");
	
	font_y_screen = font_y_pos;
}

//---------------------------------------------------------------------------------
static bool displaychan(u64 chantitle, u64 *title_list, u32 count)
//---------------------------------------------------------------------------------
{
	int i;
	
	if ((chantitle & 0xff) == 0x41)
		for (i = 0; i < count; i++)
			if ((chantitle & 0xffffff00) == (title_list[i] & 0xffffff00) &&
				(title_list[i] & 0xff) != 0x41)
				return FALSE;
	if ((chantitle & 0xffffff00) >> 8 == 0x484141)
		for (i = 0; i < count; i++)
			if ((title_list[i] & 0xffffff00) >> 8 == 0x484159 &&
				(chantitle & 0xff) == (title_list[i] & 0xff))
				return FALSE;
	if (TITLE_UPPER(chantitle) == 0x00010001 || TITLE_UPPER(chantitle) == 0x00010002 ||
		TITLE_UPPER(chantitle) == 0x00010004)
		if ((chantitle & 0xffffff00) >> 8 != 0x484142 &&
			(chantitle & 0xffffff00) >> 8 != 0x484347 &&
			TITLE_LOWER(chantitle) != 0x48415858 &&
			TITLE_LOWER(chantitle) != 0x4A4F4449)
			return TRUE;
	return FALSE;
}

//---------------------------------------------------------------------------------
static bool displaysave(u64 chantitle, u64 *title_list, u32 count)
//---------------------------------------------------------------------------------
{
	int i;
	
	if ((chantitle & 0xff) == 0x41)
		for (i = 0; i < count; i++)
			if ((chantitle & 0xffffff00) == (title_list[i] & 0xffffff00) &&
				(title_list[i] & 0xff) != 0x41)
				return FALSE;
	if ((chantitle & 0xffffff00) >> 8 == 0x484141)
		for (i = 0; i < count; i++)
			if ((title_list[i] & 0xffffff00) >> 8 == 0x484159 &&
				(chantitle & 0xff) == (title_list[i] & 0xff))
				return FALSE;
	if (TITLE_UPPER(chantitle) == 0x00010000 || TITLE_UPPER(chantitle) == 0x00010001 ||
		TITLE_UPPER(chantitle) == 0x00010002 || TITLE_UPPER(chantitle) == 0x00010004)
		if ((chantitle & 0xffffff00) >> 8 != 0x484142 &&
			(chantitle & 0xffffff00) >> 8 != 0x484347 &&
			TITLE_LOWER(chantitle) != 0x48415858)
			return TRUE;
	return FALSE;
}

//---------------------------------------------------------------------------------
static char* chanidtoname(char *chanid, u64 chantitle, char *menuname, char *chandb)
//---------------------------------------------------------------------------------
{
	s32 ret, specialchar = 0;
	u32 chandblen, i, numletters = 0, numlettersmenu = 0;
	u8 addcolon = 0;
	char *newname;
	int newnameidx = 0;
	char *chanBanner, *namedata;
	
	if (chandb == NULL)
		goto getchanname;
	
	chandblen = strlen(chandb);
	
	if (chandblen > 4)
	{
		for (i=0; i<chandblen-3; i++)
		{
			if ((i == 0 || chandb[i-1] == 10 || chandb[i-1] == 13) &&
				chanid[0] == chandb[i] &&
				chanid[1] == chandb[i+1] &&
				chanid[2] == chandb[i+2] &&
				chandb[i+3] == '-')
			{
				i += 4;
				newname = malloc(71);
				while (newnameidx < 64 &&
					i < chandblen &&
					chandb[i] != 10 &&
					chandb[i] != 13)
					newname[newnameidx++] = chandb[i++];
				newname[newnameidx] = 0;
				if (menuname != NULL) free(menuname);
			addregion:
				i = strlen(newname);
				while (i > 0 && newname[i-1] == ' ') i--;
				newname[i] = 0;
				if (chanid[3] == 'J')
					strcat(newname, " (JP)");
				else if (chanid[3] == 'P')
					strcat(newname, " (EU)");
				else if (chanid[3] == 'E')
					strcat(newname, " (US)");
				else if (chanid[3] == 'D')
					strcat(newname, " (DE)");
				else if (chanid[3] == 'F')
					strcat(newname, " (FR)");
				else
					sprintf(newname, "%s (%c)", newname, chanid[3]);
				free(chanid);
				return newname;
			}
		}
	}
	
getchanname:
	
	if(ES_SetUID(chantitle) < 0)
	{
		newname = NULL;
		goto checkmenuname;
	}
	
	chanBanner = (char *)memalign(32, 256);
	if (chanBanner == NULL)
	{
		newname = NULL;
		goto checkmenuname;
	}
	memset(chanBanner, 0, 256);
	
	if(ES_GetDataDir(chantitle, chanBanner) < 0)
	{
		free(chanBanner);
		newname = NULL;
		goto checkmenuname;
	}
	
	namedata = (char *) memalign(32, 0x140);
	if (namedata == NULL)
	{
		free(chanBanner);
		newname = NULL;
		goto checkmenuname;
	}
	memset(namedata, 0, 0x140);
	
	strcat(chanBanner, "/banner.bin");
	
	ret = ISFS_Open(chanBanner, ISFS_OPEN_READ);
	if (ret < 0)
	{
		free(chanBanner);
		free(namedata);
		newname = NULL;
		goto checkmenuname;
	}
	
	if (ISFS_Read(ret, (u8 *) namedata, 0x140) < 0)
	{
		ISFS_Close(ret);
		free(chanBanner);
		free(namedata);
		newname = NULL;
		goto checkmenuname;
	}
	
	ISFS_Close(ret);
	
	newname = malloc(87);
	memset(newname, 0, 87);
	newname[79] = 0;
	for (i = 0; (i + specialchar) < 80 && (i < 40 || (addcolon == 2 && i < 80)); i++)
	{
		if (namedata[i * 2 + 0x20] != 0 || namedata[i * 2 + 0x21] != 0)
		{
			if (addcolon == 1 && (i + specialchar) < 77 && namedata[i * 2 + 0x20] == 0 && namedata[i * 2 + 0x21] >= 'A' && namedata[i * 2 + 0x21] <= 'Z')
			{
				addcolon = 2;
				newname[i+specialchar] = ':';
				specialchar++;
				newname[i+specialchar] = ' ';
				specialchar++;
			}
			else if (addcolon == 1)
				addcolon = 2;
			if (namedata[i * 2 + 0x20] == 0x21 && namedata[i * 2 + 0x21] == 0x61 && (i + specialchar) < 78)
			{
				newname[i+specialchar] = 'I';
				specialchar++;
				newname[i+specialchar] = 'I';
			}
			else if (namedata[i * 2 + 0x20] == 0 && namedata[i * 2 + 0x21] >= 32 && namedata[i * 2 + 0x21] <= 126)
				newname[i+specialchar] = namedata[i * 2 + 0x21];
			else
				specialchar--;
		}
		else
		{
			if (addcolon != 2)
			{
				addcolon = 1;
				specialchar--;
			}
			else
			{
				newname[i+specialchar] = 0;
				break;
			}
		}
	}
	newname[80+specialchar] = 0;
	
	free(chanBanner);
	free(namedata);
	
	for (i = 0; i < 50; i++)
	{
		if ((newname[i] >= 'A' && newname[i] <= 'Z') ||
			(newname[i] >= 'a' && newname[i] <= 'z'))
			numletters++;
		else if (newname[i] == 0)
			break;
	}
	
checkmenuname:
	
	if (menuname != NULL)
	{
		for (i = 0; i < 50; i++)
		{
			if ((menuname[i] >= 'A' && menuname[i] <= 'Z') ||
				(menuname[i] >= 'a' && menuname[i] <= 'z'))
				numlettersmenu++;
			else if (menuname[i] == 0)
				break;
		}
	}
	
	if (numletters > numlettersmenu && numletters >= 3)
	{
		if (menuname != NULL) free(menuname);
		goto addregion;
	}
	else if (numlettersmenu >= 3)
	{
		if (newname != NULL) free(newname);
		newname = menuname;
		goto addregion;
	}
	
	if (newname != NULL) free(newname);
	if (menuname != NULL) free(menuname);
	return chanid;
}

//---------------------------------------------------------------------------------
s32 menu_generatechannellist(bool preinit)
//---------------------------------------------------------------------------------
{
	s32 ret, specialchar;
	u32 count, removecount = 0;
	u8 addcolon;
	static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	static u64 title_list[256] ATTRIBUTE_ALIGN(32);
	u32 contentbannerlist[256];
	signed_blob *s_tmd;
	u16 bootindex;
	u32 tmd_size, bootid, i, j, filesize, numnonascii;
	u64 chantitle;
	char *channames[256], *chandb = NULL, *databuf;
	char *filename;
	FILE *fp;
	
//	if (!preinit)
		//sdio_Shutdown();
//	else
	if (preinit)
		IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
	if (identify_SU() < 0)
		return 0;
	if (ISFS_Initialize())
		return 0;
	
	channellist_menu_items = 0;
	
	ret = ES_GetNumTitles(&count);
	if (ret) {
		return 0;
	}
	
	ret = ES_GetTitles(title_list, count);
	if (ret) {
		return 0;
	}
	
	filename = (char *)memalign(32, 256);
	if (filename == NULL) return 0;
	
	for (i=0; i < count; i++)
	{
		ret = ES_GetStoredTMDSize(title_list[i], &tmd_size);
		s_tmd = (signed_blob *)tmd_buf;
		ret = ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);
		bootindex = ((tmd *)SIGNATURE_PAYLOAD(s_tmd))->boot_index;
		for (j = 0; j < ((tmd *)SIGNATURE_PAYLOAD(s_tmd))->num_contents; j++)
		{
			if (((tmd *)SIGNATURE_PAYLOAD(s_tmd))->contents[j].index == 0)
				contentbannerlist[i] = ((tmd *)SIGNATURE_PAYLOAD(s_tmd))->contents[j].cid;
			if (((tmd *)SIGNATURE_PAYLOAD(s_tmd))->contents[j].index == bootindex)
				bootid = ((tmd *)SIGNATURE_PAYLOAD(s_tmd))->contents[j].cid;
		}
		snprintf(filename, 256, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(title_list[i]), TITLE_LOWER(title_list[i]), bootid);
		ret = ISFS_Open(filename, ISFS_OPEN_READ);
		if (ret >= 0)
		{
			contentbannerlist[i-removecount] = contentbannerlist[i];
			title_list[i-removecount] = title_list[i];
			ISFS_Close(ret);
		}
		else
			removecount++;
	}
	count -= removecount;
	
	databuf = (char *) memalign(32, 0x140);
	
	for (i=0; i < count; i++)
	{
		chantitle = title_list[i];
		snprintf(filename, 256, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(chantitle), TITLE_LOWER(chantitle), contentbannerlist[i]);
		if (databuf == NULL)
			channames[i] = NULL;
		else
		{
			ret = ISFS_Open(filename, ISFS_OPEN_READ);
			if (ret < 0)
				channames[i] = NULL;
			else
			{
				if (ISFS_Read(ret, (u8 *) databuf, 0x140) < 0)
				{
					ISFS_Close(ret);
					free(databuf);
					channames[i] = NULL;
				}
				else
				{
					ISFS_Close(ret);
					channames[i] = (char *) malloc(87);
					memset(channames[i], 0, 87);
					if (channames[i] != NULL)
					{
						(channames[i])[79] = 0;
						specialchar = 0;
						addcolon = 0;
						for (j = 0; (j + specialchar) < 80 && (j < 40 || (addcolon == 2 && j < 80)); j++)
						{
							if (databuf[j * 2 + 0xF0] != 0 || databuf[j * 2 + 0xF1] != 0)
							{
								if (addcolon == 1 && (j + specialchar) < 77 && databuf[j * 2 + 0xF0] == 0 && databuf[j * 2 + 0xF1] >= 'A' && databuf[j * 2 + 0xF1] <= 'Z')
								{
									addcolon = 2;
									(channames[i])[j+specialchar] = ':';
									specialchar++;
									(channames[i])[j+specialchar] = ' ';
									specialchar++;
								}
								else if (addcolon == 1)
									addcolon = 2;
								if (databuf[j * 2 + 0xF0] == 0x21 && databuf[j * 2 + 0xF1] == 0x61 && (j + specialchar) < 78)
								{
									(channames[i])[j+specialchar] = 'I';
									specialchar++;
									(channames[i])[j+specialchar] = 'I';
								}
								else if (databuf[j * 2 + 0xF0] == 0 && databuf[j * 2 + 0xF1] >= 32 && databuf[j * 2 + 0xF1] <= 126)
									(channames[i])[j+specialchar] = databuf[j * 2 + 0xF1];
								else
									specialchar--;
							}
							else
							{
								if (addcolon != 2)
								{
									addcolon = 1;
									specialchar--;
								}
								else
								{
									(channames[i])[j+specialchar] = 0;
									break;
								}
							}
						}
						(channames[i])[80+specialchar] = 0;
					}
				}
			}
		}
	}
	
	free(filename);
	if (databuf != NULL) free(databuf);
	
	ISFS_Deinitialize();
	if (preinit) IOS_ReloadIOS(IOS_GetVersion()); // Reset IOS, needed for some buggy cIOS versions
	Menu_identify();
	sd_init();
	ISFS_Initialize();
	
	if (sd_found == 1 || preinit)
	{
		fp = fopen("sd:/data/gecko/database.txt", "rb");
		
		if (!fp) fp = fopen("sd:/database.txt", "rb");
		
		if (fp) {
			fseek(fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			
			chandb = malloc(filesize);
			
			ret = fread((void*)chandb, 1, filesize, fp);
			fclose(fp);
			filesize = ret;
			
			// Remove non-ASCII characters
			numnonascii = 0;
			for (i = 0; i < filesize; i++)
			{
				if (chandb[i] < 9 || chandb[i] > 126)
					numnonascii++;
				else
					chandb[i-numnonascii] = chandb[i];
			}
			filesize -= numnonascii;
			chandb[filesize] = 0;
		}
	}
	
	for (i=0; i < count; i++) {
		chantitle = title_list[i];
		if (displaychan(chantitle, title_list, count))
		{
			channellistmenu[channellist_menu_items].titleid = chantitle;
			channellistmenu[channellist_menu_items].selected = 0;
			channellistmenu[channellist_menu_items].clitems = malloc(5);
			*(channellistmenu[channellist_menu_items].clitems) = (chantitle & 0xff000000) >> 24;
			*(channellistmenu[channellist_menu_items].clitems + 1) = (chantitle & 0x00ff0000) >> 16;
			*(channellistmenu[channellist_menu_items].clitems + 2) = (chantitle & 0x0000ff00) >> 8;
			*(channellistmenu[channellist_menu_items].clitems + 3) = chantitle & 0x000000ff;
			*(channellistmenu[channellist_menu_items].clitems + 4) = 0;
			channellistmenu[channellist_menu_items].clitems = chanidtoname(channellistmenu[channellist_menu_items].clitems, chantitle, channames[i], chandb);
			channellist_menu_items++;
		}
		else
			if (channames[i] != NULL) free(channames[i]);
	}
	
	ISFS_Deinitialize();
//	if (preinit) sdio_Shutdown();
	
	return 1;
}

//---------------------------------------------------------------------------------
static void menu_drawfile()
//---------------------------------------------------------------------------------
{
	menu_pad_file();

	if (!scanned){
		menu_scandir();
		scanned = 1;
	}
	
	menu_showdirlist(filelist, filetop, 0);
}


//---------------------------------------------------------------------------------
void menu_draw()
//---------------------------------------------------------------------------------
{	
	
	switch (menu_number)
	{
		case 0:
			menu_drawroot();
		break;

		case 1:
			menu_drawconfig();
		break;

		case 5:
			menu_drawfile();
		break;

		case 7:
			menu_drawtools();
		break;

		case 8:
			menu_drawapp();
		break;

		case 9:
			menu_drawextract();
		break;

		case 10:
			menu_drawhelp();
		break;

		case 11:
			menu_drawinstallios();
		break;
			
		case 12:
			menu_drawrebooter();
		break;
			
		case 13:
			menu_drawrebooterconf();
		break;
			
		case 14:
			menu_drawbootchannel();
		break;
			
		case 15:
			menu_drawchannellist();
		break;
	}

	menu_fade_selected();
}

//---------------------------------------------------------------------------------
void menu_fade_selected()
//---------------------------------------------------------------------------------
{
	if(textfade == 250){
		fadedir = 0;
	} 

	if(textfade == 0){
		fadedir = 1;
	}

	if(fadedir == 1){
		textfade = textfade +	fadevalue;
	}

	if(fadedir == 0){
		textfade = textfade -	fadevalue;
	}
}
