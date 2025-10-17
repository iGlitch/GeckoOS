#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <sys/unistd.h>

#include "apploader.h"
#include "patchcode.h"
#include "sd.h"
#include "fwrite_patch.h"
#include "fwrite_patch_slota.h"
#include "main.h"

extern void patchhook(u32 address, u32 len);
extern void patchhook2(u32 address, u32 len);
extern void patchhook3(u32 address, u32 len);

extern void multidolpatchone(u32 address, u32 len);
extern void multidolpatchtwo(u32 address, u32 len);

extern void regionfreejap(u32 address, u32 len);
extern void regionfreeusa(u32 address, u32 len);
extern void regionfreepal(u32 address, u32 len);

extern void removehealthcheck(u32 address, u32 len);

extern void copyflagcheck1(u32 address, u32 len);
extern void copyflagcheck2(u32 address, u32 len);
extern void copyflagcheck3(u32 address, u32 len);
extern void copyflagcheck4(u32 address, u32 len);
extern void copyflagcheck5(u32 address, u32 len);

extern void patchupdatecheck(u32 address, u32 len);

extern void movedvdhooks(u32 address, u32 len);

extern void multidolhook(u32 address);
extern void langvipatch(u32 address, u32 len, u8 langbyte);
extern void vipatch(u32 address, u32 len);

static const u32 multidolpatch1[2] = {
	0x3C03FFB4,0x28004F43
};	

static const u32 healthcheckhook[2] = {
	0x41810010,0x881D007D
};	

static const u32 updatecheckhook[3] = {
	0x80650050,0x80850054,0xA0A50058
};

static const u32 multidolpatch2[2] = {
	0x3F608000, 0x807B0018
};

static const u32 recoveryhooks[3] = {
	0xA00100AC,0x5400073E,0x2C00000F
};

static const u32 nocopyflag1[3] = {
	0x540007FF, 0x4182001C, 0x80630068
};

static const u32 nocopyflag2[3] = {
	0x540007FF, 0x41820024, 0x387E12E2
};

// this one is for the GH3 and VC saves
//static const u32 nocopyflag3[5] = {
//	0x2C030000, 0x40820010, 0x88010020, 0x28000002, 0x41820234
//};

static const u32 nocopyflag3[5] = {
	0x2C030000, 0x41820200,0x48000058,0x38610100
};
// this removes the display warning for no copy VC and GH3 saves
static const u32 nocopyflag4[4] = {
	0x80010008, 0x2C000000, 0x4182000C, 0x3BE00001
};

static const u32 nocopyflag5[3] = {
	0x801D0024,0x540007FF,0x41820024
};

static const u32 movedvdpatch[3] = {
	0x2C040000, 0x41820120, 0x3C608109
};

static const u32 regionfreehooks[5] = {
	0x7C600774, 0x2C000001, 0x41820030,0x40800010,0x2C000000
};

static const u32 cIOScode[16] = {
	0x7f06c378, 0x7f25cb78, 0x387e02c0, 0x4cc63182
};

static const u32 cIOSblock[16] = {
	0x2C1800F9, 0x40820008, 0x3B000024
};

static const u32 fwritepatch[8] = {
	0x9421FFD0,0x7C0802A6,0x90010034,0xBF210014,0x7C9B2378,0x7CDC3378,0x7C7A1B78,0x7CB92B78  // bushing fwrite
};

static const u32 vipatchcode[3] = {
0x4182000C,0x4180001C,0x48000018
};

const u32 viwiihooks[4] = {
	0x7CE33B78,0x38870034,0x38A70038,0x38C7004C 
};

const u32 kpadhooks[4] = {
	0x9A3F005E,0x38AE0080,0x389FFFFC,0x7E0903A6
};

const u32 kpadoldhooks[6] = {
	0x801D0060, 0x901E0060, 0x801D0064, 0x901E0064, 0x801D0068, 0x901E0068
};

const u32 joypadhooks[4] = {
	0x3AB50001, 0x3A73000C, 0x2C150004, 0x3B18000C
};

const u32 gxdrawhooks[4] = {
	0x3CA0CC01, 0x38000061, 0x3C804500, 0x98058000
};

const u32 gxflushhooks[4] = {
	0x90010014, 0x800305FC, 0x2C000000, 0x41820008
};

const u32 ossleepthreadhooks[4] = {
	0x90A402E0, 0x806502E4, 0x908502E4, 0x2C030000
};

const u32 axnextframehooks[4] = {
	0x3800000E, 0x7FE3FB78, 0xB0050000, 0x38800080
};

const u32 wpadbuttonsdownhooks[4] = {
	0x7D6B4A14, 0x816B0010, 0x7D635B78, 0x4E800020
};

const u32 wpadbuttonsdown2hooks[4] = {
	0x7D6B4A14, 0x800B0010, 0x7C030378, 0x4E800020
};

const u32 multidolhooks[4] = {
	0x7C0004AC, 0x4C00012C, 0x7FE903A6, 0x4E800420
};

const u32 multidolchanhooks[4] = {
	0x4200FFF4, 0x48000004, 0x38800000, 0x4E800020
};

const u32 langpatch[3] = {
	0x7C600775, 0x40820010, 0x38000000
};

static const u32 oldpatch002[3] = {
	0x2C000000, 0x40820214, 0x3C608000
};

static const u32 newpatch002[3] = {
	0x2C000000, 0x48000214, 0x3C608000
};

static const u32 dczeropatch[4] = {
	0x7C001FEC, 0x38630020, 0x4200FFF8, 0x4E800020
};

//---------------------------------------------------------------------------------
void dogamehooks(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		switch(config_bytes[2])
		{
		
			case 0x00:	
				
			break;

			case 0x01:	
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;

			case 0x02:
				
				if(memcmp(addr_start, kpadhooks, sizeof(kpadhooks))==0){
					patchhook((u32)addr_start, len);
				}

				if(memcmp(addr_start, kpadoldhooks, sizeof(kpadoldhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
		
			case 0x03:
				
				if(memcmp(addr_start, joypadhooks, sizeof(joypadhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
				
			case 0x04:
				
				if(memcmp(addr_start, gxdrawhooks, sizeof(gxdrawhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
				
			case 0x05:
				
				if(memcmp(addr_start, gxflushhooks, sizeof(gxflushhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
				
			case 0x06:
				
				if(memcmp(addr_start, ossleepthreadhooks, sizeof(ossleepthreadhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
				
			case 0x07:
				
				if(memcmp(addr_start, axnextframehooks, sizeof(axnextframehooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
				
			case 0x08:
				
				if(memcmp(addr_start, customhook, customhooksize)==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0){
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
				}
			break;
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void patchmenu(void *addr, u32 len, int patchnum)
//---------------------------------------------------------------------------------
{
	void *addr_start = addr;
	void *addr_end = addr+len;
	
	while(addr_start < addr_end)
	{
		switch (patchnum)
		{
			case 0:
				if(memcmp(addr_start, recoveryhooks, sizeof(recoveryhooks))==0){
					patchhook3((u32)addr_start, len);
				}
				break;
				
			case 1:
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0){
					patchhook2((u32)addr_start, len);
				}
				
				break;
				
			case 2:
				// jap region free	
				if(memcmp(addr_start, regionfreehooks, sizeof(regionfreehooks))==0){
					regionfreejap((u32)addr_start, len);
				}
				
				// usa region free
				if(memcmp(addr_start, regionfreehooks, sizeof(regionfreehooks))==0){
					regionfreeusa((u32)addr_start, len);
				}
				
				// pal region free
				if(memcmp(addr_start, regionfreehooks, sizeof(regionfreehooks))==0){
					regionfreepal((u32)addr_start, len);
				}
				
				// skip disc update
				if(memcmp(addr_start, updatecheckhook, sizeof(updatecheckhook))==0){
					patchupdatecheck((u32)addr_start, len);
				}
				break;
				
				
			case 3:
				if(memcmp(addr_start, healthcheckhook, sizeof(healthcheckhook))==0){
					removehealthcheck((u32)addr_start, len);
				}
				break;
				
				// no copy flags
			case 4:
				// Remove the actual flag so can copy back
				if(memcmp(addr_start, nocopyflag5, sizeof(nocopyflag5))==0){
					copyflagcheck5((u32)addr_start, len);
				}
				
				
				if(memcmp(addr_start, nocopyflag1, sizeof(nocopyflag1))==0){
					copyflagcheck1((u32)addr_start, len);
				}
				
				if(memcmp(addr_start, nocopyflag2, sizeof(nocopyflag2))==0){
					copyflagcheck2((u32)addr_start, len);
				}
				
				// no VC and GH3 save
				if(memcmp(addr_start, nocopyflag3, sizeof(nocopyflag2))==0){
					copyflagcheck3((u32)addr_start, len);
				}
				// no VC and GH3 save display remove
				if(memcmp(addr_start, nocopyflag4, sizeof(nocopyflag4))==0){
					copyflagcheck4((u32)addr_start, len);
				}
				
				break;
				
			case 5:
				if(memcmp(addr_start, movedvdpatch, sizeof(movedvdpatch))==0){
					movedvdhooks((u32)addr_start, len);
				}
				break;
				
				// multidol
			case 6:
				if(memcmp(addr_start, multidolpatch1, sizeof(multidolpatch1))==0){
					multidolpatchone((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolpatch2, sizeof(multidolpatch2))==0){
					multidolpatchtwo((u32)addr_start, len);
				}
				break;
			case 7:
				if(memcmp(addr_start, cIOScode, sizeof(cIOScode))==0)
					memcpy(addr_start, cIOSblock, sizeof(cIOSblock));
				break;
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void langpatcher(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, langpatch, sizeof(langpatch))==0) {
			langvipatch((u32)addr_start, len, config_bytes[0]);
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void vidolpatcher(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;
	
	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, vipatchcode, sizeof(vipatchcode))==0) {
			vipatch((u32)addr_start, len);
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void dochannelhooks(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	void *addr_start = addr;
	void *addr_end = addr+len;
	
	while(addr_start < addr_end)
	{
		switch(config_bytes[2])
		{
				
			case 0x00:	
				
				break;
				
			case 0x01:	
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x02:
				
				if(memcmp(addr_start, kpadhooks, sizeof(kpadhooks))==0){
					patchhook((u32)addr_start, len);
				}
				
				if(memcmp(addr_start, kpadoldhooks, sizeof(kpadoldhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x03:
				
				if(memcmp(addr_start, joypadhooks, sizeof(joypadhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x04:
				
				if(memcmp(addr_start, gxdrawhooks, sizeof(gxdrawhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x05:
				
				if(memcmp(addr_start, gxflushhooks, sizeof(gxflushhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x06:
				
				if(memcmp(addr_start, ossleepthreadhooks, sizeof(ossleepthreadhooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x07:
				
				if(memcmp(addr_start, axnextframehooks, sizeof(axnextframehooks))==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
				
			case 0x08:
				
				if(memcmp(addr_start, customhook, customhooksize)==0){
					patchhook((u32)addr_start, len);
				}
				if(memcmp(addr_start, multidolchanhooks, sizeof(multidolchanhooks))==0){
					*(((u32*)addr_start)+1) = 0x7FE802A6;
					DCFlushRange(((u32*)addr_start)+1, 4);
					multidolhook((u32)addr_start+sizeof(multidolchanhooks)-4);
				}
				break;
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void patchdebug(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;
	
	while(addr_start < addr_end)
	{
		
		if(memcmp(addr_start, fwritepatch, sizeof(fwritepatch))==0) {
			
			if (gecko_channel == 0)
				memcpy(addr_start,fwrite_patch_slota_bin,fwrite_patch_slota_bin_len);
			else if (gecko_channel == 1)
				memcpy(addr_start,fwrite_patch_bin,fwrite_patch_bin_len);
			// apply patch	
		}
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void determine_libogc_hook(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	void *addr_start = addr;
	void *addr_end = addr+len;
	
	config_bytes[2] = 0x09;
	
	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, wpadbuttonsdown2hooks, sizeof(wpadbuttonsdown2hooks))==0){
			config_bytes[2] = 0x0A;
			break;
		}
		
		addr_start += 4;
	}
}

//---------------------------------------------------------------------------------
void createbranch(void *addr1, void *addr2)
//---------------------------------------------------------------------------------
{
	u32 blcode;
	blcode = addr2 - addr1;
	blcode &= 0x3FFFFFF;
	blcode |= 0x48000000;
	*(u32 *)addr1 = blcode;
	DCFlushRange(addr1, 4);
}

//---------------------------------------------------------------------------------
void patchgeckomenu(void *addr, u32 len, void *menuaddr)
//---------------------------------------------------------------------------------
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;
	void *func_addr, *func_addr2;
	
	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, dczeropatch, sizeof(dczeropatch))==0) {
			createbranch(addr_start, menuaddr);
		}
		addr_start += 4;
	}
}
