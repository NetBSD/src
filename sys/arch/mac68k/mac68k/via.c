/*	$NetBSD: via.c,v 1.16 1995/03/25 22:30:39 briggs Exp $	*/

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

/*
 *	This code handles both the VIA and RBV functionality.
 */

#include <sys/param.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include "via.h"
#include <sys/kernel.h>

static int	scsi_drq_intr(void), scsi_irq_intr(void);

long	via1_noint(), via2_noint();
long	mrg_adbintr(), mrg_pmintr(), rtclock_intr(), profclock();
long	via2_nubus_intr();
int	slot_noint();
int	VIA2 = 1;		/* default for II, IIx, IIcx, SE/30. */

long (*via1itab[7])()={
	via1_noint,
	via1_noint,
	mrg_adbintr,
	via1_noint,
	mrg_pmintr,
	via1_noint,
	rtclock_intr,
};	/* VIA1 interrupt handler table */

long (*via2itab[7])()={
	(long (*)()) scsi_drq_intr,
	via2_nubus_intr,
	via2_noint,
	(long (*)()) scsi_irq_intr,
	via2_noint,	/* snd_intr */
	via2_noint,	/* via2t2_intr */
	via2_noint,
};	/* VIA2 interrupt handler table */

void		via2_intr(struct frame *);
void		rbv_intr(struct frame *);

static int	via_inited=0;
void		(*real_via2_intr)(struct frame *);
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

	/* turn off timer latch */
	via_reg(VIA1, vACR) &= 0x3f;

	if(VIA2 == VIA2OFF){
		/* Initialize VIA2 */
		via_reg(VIA2, vT1L) = 0;
		via_reg(VIA2, vT1LH) = 0;
		via_reg(VIA2, vT1C) = 0;
		via_reg(VIA2, vT1CH) = 0;
		via_reg(VIA2, vT2C) = 0;
		via_reg(VIA2, vT2CH) = 0;

		/* turn off timer latch */
		via_reg(VIA2, vACR) &= 0x3f;

#if 1
		/* unlock nubus */
		via_reg(VIA2, vPCR)   = 0x06;
		via_reg(VIA2, vBufB) |= 0x02;
		via_reg(VIA2, vDirB) |= 0x02;
#endif
		real_via2_intr = via2_intr;
		via2itab[1] = via2_nubus_intr;

	}else{	/* RBV */
		/* I'm sure that I'll find something to put in here
			someday. -- BG */
		/* enable specific interrupts */
		/* via_reg(VIA2, rIER) = RBV_INTS | 0x80; */
	}
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

	if (intbits == 0)
		return;

	if (intbits & 0x04) {
		asm("movw #0x2400, sr");
	}

	via_reg(VIA1, vIFR) = intbits;

	bitmsk = 1;
	bitnum = 0;
	mac68k_trip_debugger = 0;
	while(bitnum < 7){
		if(intbits & bitmsk){
			intpend |= bitmsk;	/* don't process this twice */
			via1itab[bitnum](bitnum);	/* run interrupt handler */
			intpend &= ~bitmsk;	/* fix previous pending */
		}
		bitnum++;
		bitmsk <<= 1;
	}
#ifdef DDB
   	if (mac68k_trip_debugger) Debugger();
#endif
}

void
via2_intr(struct frame *fp)
{
	register unsigned char	intbits;
	register char		bitnum, bitmsk;

	intbits = via_reg(VIA2, vIFR);	/* get interrupts pending */
	intbits &= via_reg(VIA2, vIER);	/* only care about enabled */
	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(VIA2, vIFR) = intbits;

	bitmsk = 0x40;
	bitnum = 7;
	while(bitnum--){
		if(intbits & bitmsk){
			via2itab[bitnum](bitnum);
		}
		bitmsk >>= 1;
	}
}

void rbv_intr(struct frame *fp)
{
	register unsigned char	intbits;
	register char		bitnum, bitmsk;

	intbits = via_reg(VIA2, rIFR);	/* get interrupts pending */
	intbits &= via_reg(VIA2, rIER);	/* only care about enabled */
	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(VIA2, rIFR) = intbits;

	bitmsk = 0x40;
	bitnum = 7;
	while(bitnum--){
		if(intbits & bitmsk){
			via2itab[bitnum](bitnum);
		}
		bitmsk >>= 1;
	}
}

long via1_noint(int bitnum)
{
  /* printf("via1_noint(%d)\n", bitnum); */
  return 1;
}

long via2_noint(int bitnum)
{
  /* printf("via2_noint(%d)\n", bitnum); */
  return 1;
}

static int	nubus_intr_mask = 0;

int
add_nubus_intr(addr, func, unit)
int addr;
int (*func)();
int unit;
{
	int	s = splhigh();
	int	slot;

	slot = nubus_addr_to_slot(addr);
	if (slot < 0) return 0;

	slotitab[slot-9] = func;
	slotutab[slot-9] = unit;

	nubus_intr_mask |= (1 << (slot-9));

	if (VIA2 == VIA2OFF) {
		via_reg(VIA2, vIER) = V2IF_SLOTINT | 0x80;
	} else {
		via_reg(VIA2, rIER) = V2IF_SLOTINT | 0x80;
	}
	splx(s);
	return 1;
}

long
via2_nubus_intr(int bit)
{
	register int	i, mask, ints, cnt=0;

try_again:
	via_reg(VIA2, vIFR) = V2IF_SLOTINT;
	if (ints = ((~via_reg(VIA2, vBufA)) & nubus_intr_mask)) {
		cnt = 0;
		mask = (1 << 5);
		i = 6;
		while (i--) {
			if (ints & mask) {
				(*slotitab[i])(slotutab[i], i+9);
			}
			mask >>= 1;
		}
	} else {
		delay(20); /* Just a delay for the fun of it. */
		if (cnt++ >= 2) {
			return 1;
		}
	}
	goto try_again;
}

long
rbv_nubus_intr(int bit)
{
	register int	i, mask, ints, cnt=0;;

try_again:
	via_reg(VIA2, rIFR) = V2IF_SLOTINT;
	if (ints = ((~via_reg(VIA2, rSlotInt)) & nubus_intr_mask)) {
		cnt = 0;
		mask = (1 << 5);
		i = 6;
		while (i--) {
			if (ints & mask) {
				(*slotitab[i])(slotutab[i], i+9);
			}
			mask >>= 1;
		}
	} else {
		delay(20); /* Just a delay for the fun of it. */
		if (cnt++ >= 2) {
			return 1;
		}
	}
	goto try_again;
}

int
slot_noint(int unit, int slot)
{
	printf("slot_noint() slot %x\n", slot);
}


void via_shutdown()
{
	if(VIA2 == VIA2OFF){
		via_reg(VIA2, vDirB) |= 0x04;  /* Set write for bit 2/
		via_reg(VIA2, vBufB) &= ~0x04; /* Shut down */
	}else if(VIA2 == RBVOFF){
		via_reg(VIA2, rBufB) &= ~0x04;
	}
}

int rbv_vidstatus(void)
{
	int montype;

	montype = via_reg(VIA2, rMonitor) & RBVMonitorMask;
	if(montype == RBVMonIDNone)
		montype = RBVMonIDOff;

   ddprintf("Monitor type: %d\n", montype);

   return(0);
	return(montype);
}

static int
scsi_irq_intr(void)
{
	if (mac68k_machine.scsi96) {
		if (ncr53c96_irq_intr()) return 1;
	}
	if (mac68k_machine.scsi80) {
		if (ncr5380_irq_intr()) return 1;
	}
	return 0;
}

static int
scsi_drq_intr(void)
{
	if (mac68k_machine.scsi96) {
		if (ncr53c96_drq_intr()) return 1;
	}
	if (mac68k_machine.scsi80) {
		if (ncr5380_drq_intr()) return 1;
	}
	return 0;
}
