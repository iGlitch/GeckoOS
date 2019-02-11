#ifndef _tools_h_
#define _tools_h_
//---------------------------------------------------------------------------------
u16 be16(const u8 *p);
u32 be32(const u8 *p);
u64 be64(const u8 *p);
void wbe16(u8 *p, u16 x);
void wbe32(u8 *p, u32 x);
void hexdump(void *d, int len);
u32 read32(u32 addr);
u16 read16(u32 addr);
void write32(u32 addr, u32 x);
void exitme();
//---------------------------------------------------------------------------------
#endif 
//---------------------------------------------------------------------------------
