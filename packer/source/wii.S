.text

.set r0,0;   .set sp,1;   .set r2,2; .set r3,3;   .set r4,4
.set r5,5;   .set r6,6;   .set r7,7;   .set r8,8;   .set r9,9
.set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14
.set r15,15; .set r16,16; .set r17,17; .set r18,18; .set r19,19
.set r20,20; .set r21,21; .set r22,22; .set r23,23; .set r24,24
.set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29
.set r30,30; .set r31,31


.globl wii_getbroadway
wii_getbroadway:
	mfpvr   r3
	blr

.globl jumpentry
jumpentry:
	mtlr	r3		# Game Entry Point in r3
	#lis	r4, 0x8000
	#ori	r4, r4, 0x18A8  # handler
	#mtctr	r4
	#btctr	r4
	blr