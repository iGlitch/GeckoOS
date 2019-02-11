#ifndef __APPLOADER_H__
#define __APPLOADER_H__

#include "sd.h"

//---------------------------------------------------------------------------------
u32 dvd_switchios();
void apploader_thread();
void apploader_thread_close();
void load_handler();
void app_copycodes(char *filename);
void __dvd_readidcb(s32 result);
void app_loadgameconfig(char *gameid);
void app_dopatch(char *filename);
void app_pokevalues();
void app_apply_patch();
void set_default_vidmode();
void process_config_vidmode();
void nothing();
//---------------------------------------------------------------------------------
struct _toc
{
	u32 bootInfoCnt;
	u32 partInfoOff;
};

struct _pinfo
{
	u32 offset;
	u32 len;
};
//---------------------------------------------------------------------------------
extern u8 iosversion;
extern u8 fakeiosversion;
extern u8 no_patches;
extern u32 patch_dest;
extern u32 patch_size;
extern int app_running;
extern int app_thread_state;
extern lwp_t thread;
extern struct wadinfo *ioswad;
extern char searchIOS[8];
extern u8 *tempcodelist;
extern u8 *codelist;
extern u8 *codelistend;
extern u32 codelistsize;
extern int codes_state;
extern int patch_state;
extern char *tempgameconf;
extern u32 *gameconf;
extern u32 tempgameconfsize;
extern u32 gameconfsize;
extern u32 customhook[8];
extern u8 customhooksize;
extern u8 willswitchios;
extern u8 configwarn;
extern u8 diagcreate;
extern u8 applyfwritepatch;
extern u8 dumpmaindol;
extern u8 vipatchon;
extern int app_x_screen;
extern int app_y_screen;
extern char gameidbuffer[8];
extern char filepath[MAX_FILEPATH_LEN];
extern GXRModeObj *rmode;
extern vu32 dvddone;
extern u32 appentrypoint;
extern dvddiskid *g_diskID;
extern u8 dvdios;
extern char _gameTocBuffer[0x20];
extern char _gamePartInfo[0x20];
extern char _gameTmdBuffer[18944];
extern u32 *__dvd_Tmd;
extern struct _toc *__dvd_gameToc;
extern struct _pinfo *__dvd_partInfo;
extern struct _pinfo *__dvd_bootGameInfo;
//---------------------------------------------------------------------------------


#endif
