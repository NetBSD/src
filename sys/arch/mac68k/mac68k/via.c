/*	$NetBSD: via.c,v 1.30 1995/09/02 19:27:48 briggs Exp $	*/

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
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include "via.h"

#include "ncrscsi.h"
#include "ncr96scsi.h"

static void	via1_noint(), via2_noint();
void	mrg_adbintr(), mrg_pmintr(), rtclock_intr(), profclock();
void	via2_nubus_intr();
void	rbv_nubus_intr();
void	slot_noint(void *, int);
int	VIA2 = 1;		/* default for II, IIx, IIcx, SE/30. */

void (*via1itab[7])()={
	via1_noint,
	via1_noint,
	mrg_adbintr,
	via1_noint,
	mrg_pmintr,
	via1_noint,
	rtclock_intr,
};	/* VIA1 interrupt handler table */

void (*via2itab[7])()={
	via2_noint,
	via2_nubus_intr,
	via2_noint,
	via2_noint,
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
void (*slotitab[6])(void *, int) = {
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint
};

void	*slotptab[6];

void
VIA_initialize()
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

	Via2Base = Via1Base + VIA2 * 0x2000;
	if (VIA2 == VIA2OFF) {
		/* Initialize VIA2 */
		via2_reg(vT1L) = 0;
		via2_reg(vT1LH) = 0;
		via2_reg(vT1C) = 0;
		via2_reg(vT1CH) = 0;
		via2_reg(vT2C) = 0;
		via2_reg(vT2CH) = 0;

		/* turn off timer latch */
		via2_reg(vACR) &= 0x3f;

		/*
		 * Turn off SE/30 video interrupts.
		 */
		if (mac68k_machine.machineid == MACH_MACSE30) {
			via_reg(VIA1, vBufB) |= (0x40);
			via_reg(VIA1, vDirB) |= (0x40);
		}

		/*
		 * unlock nubus
		 */
		via2_reg(vPCR)   = 0x66;
		via2_reg(vBufB) |= 0x02;
		via2_reg(vDirB) |= 0x02;

		real_via2_intr = via2_intr;
		via2itab[1] = via2_nubus_intr;

	} else {	/* RBV */
		if (current_mac_model->class == MACH_CLASSIIci) {
			/*
			 * Disable cache card. (p. 174--GtMFH)
			 */
			via2_reg(rBufB) |= DB2O_CEnable;
		}
		real_via2_intr = rbv_intr;
		via2itab[1] = rbv_nubus_intr;
	}
	via_inited = 1;
}


void
via1_intr(struct frame *fp)
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

	via_reg(VIA1, vIFR) = intbits;

	bitmsk = 1;
	bitnum = 0;
	mac68k_trip_debugger = 0;
	while(bitnum < 7){
		if(intbits & bitmsk){
			intpend |= bitmsk;	/* don't process this twice */
			via1itab[bitnum](bitnum); /* run interrupt handler */
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

	intbits = via2_reg(vIFR);	/* get interrupts pending */
	intbits &= via2_reg(vIER);	/* only care about enabled */
	/*
	 * Unflag interrupts we're about to process.
	 */
	via2_reg(vIFR) = intbits;

	bitmsk = 0x01;
	bitnum = 7;
	while(bitnum--){
		if(intbits & bitmsk){
			via2itab[6-bitnum](6-bitnum);
		}
		bitmsk <<= 1;
	}
}

void
rbv_intr(struct frame *fp)
{
	register unsigned char	intbits;
	register char		bitnum, bitmsk;

	intbits = (via2_reg(vIFR + rIFR) &= via2_reg(vIER + rIER));
	/*
	 * Unflag interrupts we're about to process.
	 */
	if (intbits == 0) return;

	via2_reg(rIFR) = intbits;

	bitmsk = 0x01;
	bitnum = 7;
	while(bitnum--){
		if(intbits & bitmsk){
			via2itab[6-bitnum](6-bitnum);
		}
		bitmsk <<= 1;
	}
}

static void
via1_noint(int bitnum)
{
  printf("via1_noint(%d)\n", bitnum);
}

static void
via2_noint(int bitnum)
{
  printf("via2_noint(%d)\n", bitnum);
}

static int	nubus_intr_mask = 0;

int
add_nubus_intr(slot, func, client_data)
int	slot;
void	(*func)();
void	*client_data;
{
	int	s = splhigh();

	if (slot < 9 || slot > 15) return 0;

	slotitab[slot-9] = func;
	slotptab[slot-9] = client_data;

	nubus_intr_mask |= (1 << (slot-9));

	/*
	 * The following should be uncommented and the call in if_ae.c
	 * removed when we can reliably handle interrupts from the video
	 * cards.
	 */
/*	enable_nubus_intr();	*/

	splx(s);
	return 1;
}

void
enable_nubus_intr(void)
{
	if (VIA2 == VIA2OFF) {
		via2_reg(vIER) = V2IF_SLOTINT | 0x80;
	} else {
		via2_reg(rIER) = V2IF_SLOTINT | 0x80;
	}
}

void
via2_nubus_intr(int bit)
{
	register int	i, mask, ints, cnt=0;

try_again:
	via2_reg(vIFR) = V2IF_SLOTINT;
	if (ints = ((~via2_reg(vBufA)) & nubus_intr_mask)) {
		cnt = 0;
		mask = (1 << 5);
		i = 6;
		while (i--) {
			if (ints & mask) {
				(*slotitab[i])(slotptab[i], i+9);
			}
			mask >>= 1;
		}
	} else {
		delay(7);
		if (cnt++ >= 2) {
			return;
		}
	}
	goto try_again;
}

void
rbv_nubus_intr(int bit)
{
	register int	i, mask, ints, cnt=0;;

try_again:
	via2_reg(rIFR) = V2IF_SLOTINT;
	if (ints = ((~via2_reg(rBufA)) & via2_reg(rSlotInt))) {
		cnt = 0;
		mask = (1 << 5);
		i = 6;
		while (i--) {
			if (ints & mask) {
				(*slotitab[i])(slotptab[i], i+9);
			}
			mask >>= 1;
		}
	} else {
		delay(7);
		if (cnt++ >= 2)
			return;
	}
	goto try_again;
}

void
slot_noint(void *client_data, int slot)
{
	printf("slot_noint() slot %x\n", slot);
}


void
via_shutdown()
{
	if(VIA2 == VIA2OFF){
		via2_reg(vDirB) |= 0x04;  /* Set write for bit 2 */
		via2_reg(vBufB) &= ~0x04; /* Shut down */
	}else if(VIA2 == RBVOFF){
		via2_reg(rBufB) &= ~0x04;
	}
}

int
rbv_vidstatus()
{
/*
	int montype;

	montype = via2_reg(rMonitor) & RBVMonitorMask;
	if(montype == RBVMonIDNone)
		montype = RBVMonIDOff;
*/
	return(0);
}

extern void
mac68k_register_scsi_drq(drq_func)
	void	(*drq_func)(void);
{
	if (drq_func)
		via2itab[0] = drq_func;
	else
 		via2itab[0] = via2_noint;
}

extern void
mac68k_register_scsi_irq(irq_func)
	void	(*irq_func)(void);
{
	if (irq_func)
 		via2itab[3] = irq_func;
	else
 		via2itab[3] = via2_noint;
}
