#ifndef _menu_h_
#define _menu_h_
//---------------------------------------------------------------------------------
void menu_draw();
void menu_fade_selected();
void menu_load_config();
s32 menu_generatechannellist(bool preinit);
//---------------------------------------------------------------------------------
extern int scanned;
extern int idcount;
extern int file_menu_selected;
extern int config_not_loaded;
extern u8 textfade;
extern u8 rebooterasmenuitem;
extern u16 channellist_menu_items;
extern u32 langselect;
extern u32 ntscselect;
extern u32 pal60select;
extern u32 pal50select;
extern u32 hooktypeselect;
extern u32 debuggerselect;
extern u32 filepatcherselect;
extern u32 ocarinaselect;
extern u32 pausedstartselect;
extern u32 bubbleselect;
extern u32 recoveryselect;
extern u32 regionfreeselect;
extern u32 nocopyselect;
extern u32 buttonskipselect;
extern u32 menu_number;
extern u32 error_sd;
extern u32 confirm_sd;
extern u32 error_video;
//---------------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------------
