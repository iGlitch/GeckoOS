#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "relos.h"
#include "binary.h"
#include "aram.h"


#define reloc_code_location 0x817f8000
//#define binarybuffer 0x817AE000		// change if DOL is over 300K
#define binarybuffer 0x93500000	// Wii mode

//#define binarybuffer 0x80004000	// Wii mode

extern void jumpentry(u32 entryp);

int main(int argc, char *argv[])
{

	depackrnc1((u8*)binary,(void *)binarybuffer);
	DCFlushRange((void *)binarybuffer, binary_size);
	
	memcpy((unsigned char*)reloc_code_location, relos, relos_size);	// relocate DOL loader
	DCFlushRange((unsigned char*)reloc_code_location, relos_size);
	void (*exe_entry)(void*) = reloc_code_location;
	exe_entry((void*)binarybuffer);	// disables IRQ and relocates from very high
	return 0;

	return 0;
}
