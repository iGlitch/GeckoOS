#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>	
#include "aram.h"

#define ARAM_Read 1
#define ARAM_Write 0

// Write Wrapper
void write_aram(char *src, char *dst, int len)
{
    AR_StartDMA(ARAM_Write, (u32)src, (u32)dst, len);
	DCFlushRange(src, len);
	while (AR_GetDMAStatus());
}

