/*	$NetBSD: via.c,v 1.72 2003/07/15 02:43:23 lukem Exp $	*/

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
 *	This code handles VIA, RBV, and OSS functionality.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: via.c,v 1.72 2003/07/15 02:43:23 lukem Exp $");

#include "opt_mac68k.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/viareg.h>

void	mrg_adbintr __P((void *));
void	mrg_pmintr __P((void *));
void	rtclock_intr __P((void *));
void	profclock __P((void *));

void	via1_intr __P((void *));
void	via2_intr __P((void *));
void	rbv_intr __P((void *));
void	oss_intr __P((void *));
void	via2_nubus_intr __P((void *));
void	rbv_nubus_intr __P((void *));

static void	via1_noint __P((void *));
static void	via2_noint __P((void *));
static void	slot_ignore __P((void *));
static void	slot_noint __P((void *));

int	VIA2 = VIA2OFF;		/* default for II, IIx, IIcx, SE/30. */

/* VIA1 interrupt handler table */
void (*via1itab[7]) __P((void *)) = {
	via1_noint,
	via1_noint,
	mrg_adbintr,
	via1_noint,
	mrg_pmintr,
	via1_noint,
	rtclock_intr,
};

/* Arg array for VIA1 interrupts. */
void *via1iarg[7] = {
	(void *)0,
	(void *)1,
	(void *)2,
	(void *)3,
	(void *)4,
	(void *)5,
	(void *)6
};

/* VIA2 interrupt handler table */
void (*via2itab[7]) __P((void *)) = {
	via2_noint,
	via2_nubus_intr,
	via2_noint,
	via2_noint,
	via2_noint,	/* snd_intr */
	via2_noint,	/* via2t2_intr */
	via2_noint,
};

/* Arg array for VIA2 interrupts. */
void *via2iarg[7] = {
	(void *)0,
	(void *)1,
	(void *)2,
	(void *)3,
	(void *)4,
	(void *)5,
	(void *)6
};

/*
 * Nubus slot interrupt routines and parameters for slots 9-15.  Note
 * that for simplicity of code, "v2IRQ0" for internal video is treated
 * as a slot 15 interrupt; this slot is quite fictitious in real-world
 * Macs.  See also GMFH, pp. 165-167, and "Monster, Loch Ness."
 */
void (*slotitab[7]) __P((void *)) = {
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint,
	slot_noint	/* int_video_intr */
};

void *slotptab[7] = {
	(void *)0,
	(void *)1,
	(void *)2,
	(void *)3,
	(void *)4,
	(void *)5,
	(void *)6
};

static int	nubus_intr_mask = 0;

void
via_init()
{
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

	intr_establish((int (*)(void *)) via1_intr, NULL, mac68k_machine.via1_ipl);

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
		 * Set vPCR for SCSI interrupts.
		 */
		via2_reg(vPCR) = 0x66;
		switch(mac68k_machine.machineid) {
		case MACH_MACPB140:
		case MACH_MACPB145:
		case MACH_MACPB150:
		case MACH_MACPB160:
		case MACH_MACPB165:
		case MACH_MACPB165C:
		case MACH_MACPB170:
		case MACH_MACPB180:
		case MACH_MACPB180C:
			break;
		default:
			via2_reg(vBufB) |= 0x02;	/* Unlock NuBus */
			via2_reg(vDirB) |= 0x02;
			break;
		}

		intr_establish((int (*)(void*))via2_intr, NULL,
				mac68k_machine.via2_ipl);
		via2itab[1] = via2_nubus_intr;
	} else if (current_mac_model->class == MACH_CLASSIIfx) { /* OSS */
		volatile u_char *ossintr;
		ossintr = (volatile u_char *)IOBase + 0x1a006;
		*ossintr = 0;
		intr_establish((int (*)(void*))oss_intr, NULL,
	 			mac68k_machine.via2_ipl);
	} else {	/* RBV */
#ifdef DISABLE_EXT_CACHE
		if (current_mac_model->class == MACH_CLASSIIci) {
			/*
			 * Disable cache card. (p. 174 -- GMFH)
			 */
			via2_reg(rBufB) |= DB2O_CEnable;
		}
#endif
		intr_establish((int (*)(void*))rbv_intr, NULL,
				mac68k_machine.via2_ipl);
		via2itab[1] = rbv_nubus_intr;
		add_nubus_intr(0, slot_ignore, NULL);
	}
}

/*
 * Set the state of the modem serial port's clock source.
 */
void
via_set_modem(onoff)
	int	onoff;
{
	via_reg(VIA1, vDirA) |= DA1O_vSync;
	if (onoff)
		via_reg(VIA1, vBufA) |= DA1O_vSync;
	else
		via_reg(VIA1, vBufA) &= ~DA1O_vSync;
}

void
via1_intr(intr_arg)
	void *intr_arg;
{
	u_int8_t intbits, bitnum;
	u_int mask;

	intbits = via_reg(VIA1, vIFR);		/* get interrupts pending */
	intbits &= via_reg(VIA1, vIER);		/* only care about enabled */

	if (intbits == 0)
		return;

	/*
	 * Unflag interrupts here.  If we do it after each interrupt,
	 * the MRG ADB hangs up.
	 */
	via_reg(VIA1, vIFR) = intbits;

	intbits &= 0x7f;
	mask = 1;
	bitnum = 0;
	do {
		if (intbits & mask) {
			via1itab[bitnum](via1iarg[bitnum]);
			/* via_reg(VIA1, vIFR) = mask; */
		}
		mask <<= 1;
		++bitnum;
	} while (intbits >= mask);
}

void
via2_intr(intr_arg)
	void *intr_arg;
{
	u_int8_t intbits, bitnum;
	u_int mask;

	intbits = via2_reg(vIFR);		/* get interrupts pending */
	intbits &= via2_reg(vIER);		/* only care about enabled */

	if (intbits == 0)
		return;

	via2_reg(vIFR) = intbits;

	intbits &= 0x7f;
	mask = 1;
	bitnum = 0;
	do {
		if (intbits & mask)
			via2itab[bitnum](via2iarg[bitnum]);
		mask <<= 1;
		++bitnum;
	} while (intbits >= mask);
}

void
rbv_intr(intr_arg)
	void *intr_arg;
{
	u_int8_t intbits, bitnum;
	u_int mask;

	intbits = (via2_reg(vIFR + rIFR) & via2_reg(vIER + rIER));

	if (intbits == 0)
		return;

	via2_reg(rIFR) = intbits;

	intbits &= 0x7f;
	mask = 1;
	bitnum = 0;
	do {
		if (intbits & mask)
			via2itab[bitnum](via2iarg[bitnum]);
		mask <<= 1;
		++bitnum;
	} while (intbits >= mask);
}

void
oss_intr(intr_arg)
	void *intr_arg;
{
	u_int8_t intbits, bitnum;
	u_int mask;

	intbits = via2_reg(vIFR + rIFR);

	if (intbits == 0)
		return;

	intbits &= 0x7f;
	mask = 1;
	bitnum = 0;
	do {
		if (intbits & mask) {
			(*slotitab[bitnum])(slotptab[bitnum]);
			via2_reg(rIFR) = mask;
		}
		mask <<= 1;
		++bitnum;
	} while (intbits >= mask);
}

static void
via1_noint(bitnum)
	void *bitnum;
{
	printf("via1_noint(%d)\n", (int)bitnum);
}

static void
via2_noint(bitnum)
	void *bitnum;
{
	printf("via2_noint(%d)\n", (int)bitnum);
}

int
add_nubus_intr(slot, func, client_data)
	int slot;
	void (*func) __P((void *));
	void *client_data;
{
	int	s;

	/*
	 * Map Nubus slot 0 to "slot" 15; see note on Nubus slot
	 * interrupt tables.
	 */
	if (slot == 0)
		slot = 15;
	if (slot < 9 || slot > 15)
		return 0;

	s = splhigh();

	if (func == NULL) {
		slotitab[slot - 9] = slot_noint;
		nubus_intr_mask &= ~(1 << (slot - 9));
	} else {
		slotitab[slot - 9] = func;
		nubus_intr_mask |= (1 << (slot - 9));
	}
	if (client_data == NULL)
		slotptab[slot - 9] = (void *)(slot - 9);
	else
		slotptab[slot - 9] = client_data;

	splx(s);

	return 1;
}

void
enable_nubus_intr()
{
	if ((nubus_intr_mask & 0x3f) == 0)
		return;

	if (VIA2 == VIA2OFF)
		via2_reg(vIER) = 0x80 | V2IF_SLOTINT;
	else
		via2_reg(rIER) = 0x80 | V2IF_SLOTINT;
}

/*ARGSUSED*/
void
via2_nubus_intr(bitarg)
	void *bitarg;
{
	u_int8_t i, intbits, mask;

	via2_reg(vIFR) = V2IF_SLOTINT;
	while ((intbits = (~via2_reg(vBufA)) & nubus_intr_mask)) {
		i = 6;
		mask = (1 << i);
		do {
			if (intbits & mask)
				(*slotitab[i])(slotptab[i]);
			i--;
			mask >>= 1;
		} while (mask);
		via2_reg(vIFR) = V2IF_SLOTINT;
	}
}

/*ARGSUSED*/
void
rbv_nubus_intr(bitarg)
	void *bitarg;
{
	u_int8_t i, intbits, mask;

	via2_reg(rIFR) = 0x80 | V2IF_SLOTINT;
	while ((intbits = (~via2_reg(rBufA)) & via2_reg(rSlotInt))) {
		i = 6;
		mask = (1 << i);
		do {
			if (intbits & mask)
				(*slotitab[i])(slotptab[i]);
			i--;
			mask >>= 1;
		} while (mask);
		via2_reg(rIFR) = 0x80 | V2IF_SLOTINT;
	}
}

static void
slot_ignore(client_data)
	void *client_data;
{
	int mask = (1 << (int)client_data);

	if (VIA2 == VIA2OFF) {
		via2_reg(vDirA) |= mask;
		via2_reg(vBufA) = mask;
		via2_reg(vDirA) &= ~mask;
	} else
		via2_reg(rBufA) = mask;
}

static void
slot_noint(client_data)
	void *client_data;
{
	int slot = (int)client_data + 9;

	printf("slot_noint() slot %x\n", slot);

	/* attempt to clear the interrupt */
	slot_ignore(client_data);
}

void
via_powerdown()
{
	if (VIA2 == VIA2OFF) {
		via2_reg(vDirB) |= 0x04;  /* Set write for bit 2 */
		via2_reg(vBufB) &= ~0x04; /* Shut down */
	} else if (VIA2 == RBVOFF) {
		via2_reg(rBufB) &= ~0x04;
	} else if (VIA2 == OSSOFF) {
		/*
		 * Thanks to Brad Boyer <flar@cegt201.bradley.edu> for the
		 * Linux/mac68k code that I derived this from.
		 */
		via2_reg(OSS_oRCR) |= OSS_POWEROFF;
	}
}

void
via1_register_irq(irq, irq_func, client_data)
	int irq;
	void (*irq_func)(void *);
	void *client_data;
{
	if (irq_func) {
 		via1itab[irq] = irq_func;
 		via1iarg[irq] = client_data;
	} else {
 		via1itab[irq] = via1_noint;
 		via1iarg[irq] = (void *)0;
	}
}

void
via2_register_irq(irq, irq_func, client_data)
	int irq;
	void (*irq_func)(void *);
	void *client_data;
{
	if (irq_func) {
 		via2itab[irq] = irq_func;
		via2iarg[irq] = client_data;
	} else {
 		via2itab[irq] = via2_noint;
		via2iarg[irq] = (void *)0;
	}
}
