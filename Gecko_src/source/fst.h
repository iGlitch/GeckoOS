#ifndef __FST_H__
#define __FST_H__


//---------------------------------------------------------------------------------
struct wadinfo
{
	char *name;
	u32 offset;
	u32 len;
};
//---------------------------------------------------------------------------------
void fst_getupdates();
s32 fst_install_wad(u32 fileoffset, u32 filelen, u64 *expected_title);
struct wadinfo* fst_find_wad(u32 fstlocation, char *wadname);
void installios_thread();
void installios_thread_close();
void fst_thread_close();
void fst_thread();
//---------------------------------------------------------------------------------
extern int real_entries;
extern int no_entries;
extern int fst_running;
extern int fst_thread_state;
extern int installios_running;
extern int installios_thread_state;
extern s32 iosinstallret;
extern lwp_t fstthread;
extern lwp_t installiosthread;
extern u64 iosinstalltitle;
extern int do_patch;
extern int tmd_dirty;
extern int tik_dirty;
//---------------------------------------------------------------------------------
#endif
