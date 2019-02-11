#ifndef __DVD_BROADWAY_H__
#define __DVD_BROADWAY_H__
 
#include <gctypes.h>
#include <ogc/ipc.h>
#include <ogc/dvd.h>
//---------------------------------------------------------------------------------
typedef void (*dvdcallbacklow)(s32 result);
 
s32 DVDLowInit();
s32 DVDLowInquiry(dvddrvinfo *info,dvdcallbacklow cb);
s32 DVDLowReadID(dvddiskid *diskID,dvdcallbacklow cb);
s32 DVDLowClosePartition(dvdcallbacklow cb);
s32 DVDLowOpenPartition(u32 offset,void *eticket,u32 certin_len,void *certificate_in,void *certificate_out,dvdcallbacklow cb);
s32 DVDLowUnencryptedRead(void *buf,u32 len,u32 offset,dvdcallbacklow cb);
s32 DVDLowReset(dvdcallbacklow cb);
s32 DVDLowWaitCoverClose(dvdcallbacklow cb);
s32 DVDLowRead(void *buf,u32 len,u32 offset,dvdcallbacklow cb);
s32 DVDEnableVideo(dvdcallbacklow cb);
s32 DVDLowReadVideo(void *buf,u32 len,u32 offset,dvdcallbacklow cb);
//--------------------------------------------------------------------------------- 
#endif
