/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "sys/param.h"
#include "machine/frame.h"
#include "via.h"
#include "kernel.h"
/* #include "stand.h" */

long via1_noint(), via2_noint();
long adb_intr(), rtclock_intr(), scsi_drq_intr(), scsi_irq_intr(), profclock();
long nubus_intr();
int  slot_noint();

long via1_spent[2][7]={
	{0,0,0,0,0,0,0}, 
	{0,0,0,0,0,0,0}, 
 };

long (*via1itab[7])()={
	via1_noint,
	via1_noint,
	adb_intr,
	via1_noint,
	via1_noint,
	via1_noint,
	rtclock_intr,
 };	/* VIA1 interrupt handler table */

long (*via2itab[7])()={
	scsi_drq_intr,
	nubus_intr,
	via2_noint,
	scsi_irq_intr,
	via2_noint, /* snd_intr */
	via2_noint, /* via2t2_intr */
#if defined(GPROF) && defined(PROFTIMER)
	profclock,
#else
	via2_noint,
#endif
 };	/* VIA2 interrupt handler table */


static int	via_inited=0;
int		mac68k_trip_debugger=0;

/* nubus slot interrupt routines */
int (*slotitab[6])() = {
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint
};

int slotutab[6];

void VIA_initialize()
{
	/* Sanity. */
	if(via_inited){printf("WARNING: Initializing VIA's again.\n");return;}

	/* Initialize VIA1 */
	/* set all timers to 0 */
	via_reg(VIA1, vT1L) = 0;
	via_reg(VIA1, vT1LH) = 0;
	via_reg(VIA1, vT1C) = 0;
	via_reg(VIA1, vT1CH) = 0;
	via_reg(VIA1, vT2C) = 0;
	via_reg(VIA1, vT2CH) = 0;

	/* program direction and data for VIA #1 */
	via_reg(VIA1, vBufA) = 0x01;
	via_reg(VIA1, vDirA) = 0x3f;
	via_reg(VIA1, vBufB) = 0x07;
	via_reg(VIA1, vDirB) = 0x87;

	/* disable all interrupts */
	via_reg(VIA1, vIFR) = 0x7f;
	via_reg(VIA1, vIER) = 0x7f;

	/* enable specific interrupts */
	/* via_reg(VIA1, vIER) = (VIA1_INTS & (~(V1IF_T1))) | 0x80; */
	via_reg(VIA1, vIER) = V1IF_ADBRDY | 0x80;
				/*  could put ^--- || V1IF_ADBRDY in here */

	/* turn off timer latch */
	via_reg(VIA1, vACR) &= 0x3f;

	/* Initialize VIA2 */
	via_reg(VIA2, vT1L) = 0;
	via_reg(VIA2, vT1LH) = 0;
	via_reg(VIA2, vT1C) = 0;
	via_reg(VIA2, vT1CH) = 0;
	via_reg(VIA2, vT2C) = 0;
	via_reg(VIA2, vT2CH) = 0;

	/* turn off timer latch */
	via_reg(VIA2, vACR) &= 0x3f;

	/* program direction and data for VIA #2 */
	via_reg(VIA2, vBufA) = via_reg(VIA2, vBufA);
	via_reg(VIA2, vDirA) = 0xc0;
	via_reg(VIA2, vBufB) = 0x05;
	via_reg(VIA2, vDirB) = 0x80;

	/* unlock nubus */
	via_reg(VIA2, vPCR)   = 0x06;
	via_reg(VIA2, vBufB) |= 0x02;
	via_reg(VIA2, vDirB) |= 0x02;

#ifdef never
	/* disable all interrupts */
	via_reg(VIA2, vIER) = 0x7f;
	via_reg(VIA2, vIFR) = 0x7f;

	/* enable specific interrupts */
	via_reg(VIA2, vIER) = VIA2_INTS | 0x80;
#endif
	via_inited=1;
}


void via1_intr(struct frame *fp)
{
	static intpend = 0;
	register unsigned char intbits, enbbits;
	register unsigned char bitnum, bitmsk;
	struct timeval before, after;

	intbits = via_reg(VIA1, vIFR);	/* get interrupts pending */
	intbits &= via_reg(VIA1, vIER);	/* only care about enabled ones */
	intbits &= ~ intpend;  		/* to stop recursion */

	bitmsk = 1;
	bitnum = 0;
	mac68k_trip_debugger = 0;
	while(bitnum < 7){
		if(intbits & bitmsk){
			intpend |= bitmsk;	/* don't process this twice */
			before = time;
			via1itab[bitnum](bitnum);	/* run interrupt handler */
			after = time;
			via1_spent[0][bitnum] += (after.tv_sec - before.tv_sec) *
			    1000000;
			if(after.tv_usec < before.tv_usec)
				via1_spent[0][bitnum] -= before.tv_usec - after.tv_usec;
			else
				via1_spent[0][bitnum] += after.tv_usec - before.tv_usec;
			via1_spent[1][bitnum]++;
			intpend &= ~bitmsk;	/* fix previous pending */
			via_reg(VIA1, vIFR) = bitmsk;
					/* turn off interrupt pending. */
		}
		bitnum++;
		bitmsk <<= 1;
	}
   	if (mac68k_trip_debugger) Debugger();
}


void via2_intr(struct frame *fp)
{
	static intpend = 0;
	register unsigned char intbits, enbbits;
	register char bitnum, bitmsk;

	intbits = via_reg(VIA2, vIFR);	/* get interrupts pending */
	intbits &= via_reg(VIA2, vIER);	/* only care about enabled ones */
	intbits &= ~ intpend;  		/* to stop recursion */

	bitmsk = 1;
	bitnum = 0;
	while(bitnum < 7){
		if(intbits & bitmsk){
			intpend |= bitmsk;	/* don't process this twice */
			via2itab[bitnum](bitnum);	/* run interrupt handler */
			intpend &= ~bitmsk;	/* fix previous pending */
			via_reg(VIA2, vIFR) = bitmsk;
					/* turn off interrupt pending. */
		}
		bitnum++;
		bitmsk <<= 1;
	}
	if (intpend) printf("via2_intr(): intpend at end 0x%x.\n", intpend);
}

long via1_noint(int bitnum)
{
  printf("via1_noint(%d)\n", bitnum);
  return 1;
}

long via2_noint(int bitnum)
{
  printf("via2_noint(%d)\n", bitnum);
  return 1;
}

int add_nubus_intr(slot, func, unit)
int slot;
int (*func)();
int unit;
{
	int s = splhigh();
	slotitab[slot-9] = func;
	slotutab[slot-9] = unit;

	via_reg(VIA2, vIER) = /*VIA2_INTS |*/ V2IF_SLOTINT | 0x80;
	splx(s);
	return 1;
}

long nubus_intr()
{
	int i, mask;

if ((~(via_reg(VIA2, vBufA) & 0x3f)) == 0x02)
	printf("slot a nu-bus intr\n");

	do {
	    mask = 1;
	    for (i = 0; i < 6; i++) {
		if ((via_reg(VIA2, vBufA) & mask) == 0)
			(*slotitab[i])(slotutab[i], i+9);
		mask <<= 1;
	    } 
	} while ((via_reg(VIA2, vBufA) & 0x3f) != 0x3f);

	return 1;
}

static char zero = 0;

int slot_noint(int unit, int slot)
{
/*
	printf("slot_noint() slot %x\n", slot);
*/
((char *)(0xf0000000 | ((long)slot << 24)))[0xa0000] = zero;
	return 1;
}


void via_shutdown()
{
  via_reg(VIA2, vDirB) |= 0x04;  /* Set write for bit 2 */
  via_reg(VIA2, vBufB) &= ~0x04; /* Shut down */
}

