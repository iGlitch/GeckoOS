#ifndef _SD_H_
#define _SD_H_

#include <fat.h>
#include <gccore.h>

#define MAX_FILENAME_LEN	128
#define MAX_FILEPATH_LEN	256

extern u32 sd_found;

//---------------------------------------------------------------------------------
void sd_refresh();
u32 sd_init();
u32 sd_shutdown();
u32 sd_load_config();
u32 sd_save_config();
void sd_check_mkdir(char *path);
//---------------------------------------------------------------------------------
#define no_config_bytes 12
extern u8 config_bytes[no_config_bytes];
//---------------------------------------------------------------------------------
#endif
