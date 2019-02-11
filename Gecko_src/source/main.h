#ifndef _main_h_
#define _main_h_

//---------------------------------------------------------------------------------
void detectIOScapabilities();
bool isIOSstub(u8 ios_number);
//---------------------------------------------------------------------------------
extern void _unstub_start();
extern void jumpentry(u32 entryp);
extern void enableLegacyDI();
extern bool sdio_Startup(void);
extern bool sdio_Deinitialize(void);
extern bool sdio_Shutdown(void);
extern void __exception_closeall();
extern s32 __IOS_InitializeSubsystems();
//---------------------------------------------------------------------------------
extern u8 gecko_command;
extern u32 gecko_attached;
extern u32 gecko_channel;
extern u8 autobootcheck;
extern u8 autoboot;
extern u8 autobootwait;
extern u8 autoboothbc;
extern u8 disableRebooter;
extern u8 buggycIOS;
extern u8 identifyworks;
extern u8 loaderhbc;
extern u8 startupiosloaded;
extern u8 progmode;
//---------------------------------------------------------------------------------
#endif 
//---------------------------------------------------------------------------------
