
#ifndef __IDENTIFY_H__
#define __IDENTIFY_H__

// Turn upper and lower into a full title ID
#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))
// Get upper or lower half of a title ID
#define TITLE_UPPER(x)		((u32)((x) >> 32))
// Turn upper and lower into a full title ID
#define TITLE_LOWER(x)		((u32)(x))

// Boot index
extern u16 bootindex;

extern u32 bootid;

// Function prototypes
void codehandler_rebooter();
void codehandler_channel();
void boot_menu();
void boot_channel();
void rebooter_thread_close();
void rebooter_thread();
void channel_thread();
void channel_thread_close();
s32 Menu_identify();
s32 ES_Load_dol(u16 index);
s32 ISFS_Load_dol(u32 cid, u64 titleid);
s32 Channel_identify();
s32 identify_SU();
u32 DIVerify_works();

// Global variables
extern int rebooter_running;
extern int rebooter_thread_state;
extern lwp_t rebooterthread;
extern u32 *bootentrypoint;
extern int channel_running;
extern int channel_thread_state;
extern lwp_t channelthread;
extern u64 channeltoload;
extern u8 channelios;
extern u8 channelidentified;

#endif // __Identify_H__

