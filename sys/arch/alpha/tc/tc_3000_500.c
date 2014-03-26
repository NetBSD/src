/* $NetBSD: tc_3000_500.c,v 1.32 2014/03/26 08:09:06 christos Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tc_3000_500.c,v 1.32 2014/03/26 08:09:06 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pte.h>
#include <machine/rpb.h>

#include <dev/tc/tcvar.h>
#include <alpha/tc/tc_conf.h>
#include <alpha/tc/tc_3000_500.h>

#include "wsdisplay.h"
#include "sfb.h"

#if NSFB > 0
extern int	sfb_cnattach(tc_addr_t);
#endif

void	tc_3000_500_intr_setup(void);
void	tc_3000_500_intr_establish(device_t, void *,
	    tc_intrlevel_t, int (*)(void *), void *);
void	tc_3000_500_intr_disestablish(device_t, void *);
void	tc_3000_500_iointr(void *, unsigned long);

int	tc_3000_500_intrnull(void *);
int	tc_3000_500_fb_cnattach(uint64_t);

#define C(x)	((void *)(u_long)x)
#define	KV(x)	(ALPHA_PHYS_TO_K0SEG(x))

struct tc_slotdesc tc_3000_500_slots[] = {
	{ KV(0x100000000), C(TC_3000_500_DEV_OPT0), },	/* 0 - opt slot 0 */
	{ KV(0x120000000), C(TC_3000_500_DEV_OPT1), },	/* 1 - opt slot 1 */
	{ KV(0x140000000), C(TC_3000_500_DEV_OPT2), },	/* 2 - opt slot 2 */
	{ KV(0x160000000), C(TC_3000_500_DEV_OPT3), },	/* 3 - opt slot 3 */
	{ KV(0x180000000), C(TC_3000_500_DEV_OPT4), },	/* 4 - opt slot 4 */
	{ KV(0x1a0000000), C(TC_3000_500_DEV_OPT5), },	/* 5 - opt slot 5 */
	{ KV(0x1c0000000), C(TC_3000_500_DEV_BOGUS), },	/* 6 - TCDS ASIC */
	{ KV(0x1e0000000), C(TC_3000_500_DEV_BOGUS), },	/* 7 - IOCTL ASIC */
};
int tc_3000_500_nslots =
    sizeof(tc_3000_500_slots) / sizeof(tc_3000_500_slots[0]);

struct tc_builtin tc_3000_500_graphics_builtins[] = {
	{ "FLAMG-IO",	7, 0x00000000, C(TC_3000_500_DEV_IOASIC),	},
	{ "PMAGB-BA",	7, 0x02000000, C(TC_3000_500_DEV_CXTURBO),	},
	{ "PMAZ-DS ",	6, 0x00000000, C(TC_3000_500_DEV_TCDS),		},
};
int tc_3000_500_graphics_nbuiltins = sizeof(tc_3000_500_graphics_builtins) /
    sizeof(tc_3000_500_graphics_builtins[0]);

struct tc_builtin tc_3000_500_nographics_builtins[] = {
	{ "FLAMG-IO",	7, 0x00000000, C(TC_3000_500_DEV_IOASIC),	},
	{ "PMAZ-DS ",	6, 0x00000000, C(TC_3000_500_DEV_TCDS),		},
};
int tc_3000_500_nographics_nbuiltins = sizeof(tc_3000_500_nographics_builtins) /
    sizeof(tc_3000_500_nographics_builtins[0]);

uint32_t tc_3000_500_intrbits[TC_3000_500_NCOOKIES] = {
	TC_3000_500_IR_OPT0,
	TC_3000_500_IR_OPT1,
	TC_3000_500_IR_OPT2,
	TC_3000_500_IR_OPT3,
	TC_3000_500_IR_OPT4,
	TC_3000_500_IR_OPT5,
	TC_3000_500_IR_TCDS,
	TC_3000_500_IR_IOASIC,
	TC_3000_500_IR_CXTURBO,
};

struct tcintr {
	int	(*tci_func)(void *);
	void	*tci_arg;
	struct evcnt tci_evcnt;
} tc_3000_500_intr[TC_3000_500_NCOOKIES];

uint32_t tc_3000_500_imask;	/* intrs we want to ignore; mirrors IMR. */

void
tc_3000_500_intr_setup(void)
{
	char *cp;
	u_long i;

	/*
	 * Disable all slot interrupts.  Note that this cannot
	 * actually disable CXTurbo, TCDS, and IOASIC interrupts.
	 */
	tc_3000_500_imask = *(volatile uint32_t *)TC_3000_500_IMR_READ;
	for (i = 0; i < TC_3000_500_NCOOKIES; i++)
		tc_3000_500_imask |= tc_3000_500_intrbits[i];
	*(volatile uint32_t *)TC_3000_500_IMR_WRITE = tc_3000_500_imask;
	tc_mb();

	/*
	 * Set up interrupt handlers.
	 */
	for (i = 0; i < TC_3000_500_NCOOKIES; i++) {
		static const size_t len = 12;
		tc_3000_500_intr[i].tci_func = tc_3000_500_intrnull;
		tc_3000_500_intr[i].tci_arg = (void *)i;

		cp = malloc(len, M_DEVBUF, M_NOWAIT);
		if (cp == NULL)
			panic("tc_3000_500_intr_setup");
		snprintf(cp, len, "slot %lu", i);
		evcnt_attach_dynamic(&tc_3000_500_intr[i].tci_evcnt,
		    EVCNT_TYPE_INTR, NULL, "tc", cp);
	}
}

const struct evcnt *
tc_3000_500_intr_evcnt(device_t tcadev, void *cookie)
{
	u_long dev = (u_long)cookie;

#ifdef DIAGNOSTIC
	/* XXX bounds-check cookie. */
#endif

	return (&tc_3000_500_intr[dev].tci_evcnt);
}

void
tc_3000_500_intr_establish(device_t tcadev, void *cookie, tc_intrlevel_t level, int (*func)(void *), void *arg)
{
	u_long dev = (u_long)cookie;

#ifdef DIAGNOSTIC
	/* XXX bounds-check cookie. */
#endif

	if (tc_3000_500_intr[dev].tci_func != tc_3000_500_intrnull)
		panic("tc_3000_500_intr_establish: cookie %lu twice", dev);

	tc_3000_500_intr[dev].tci_func = func;
	tc_3000_500_intr[dev].tci_arg = arg;

	tc_3000_500_imask &= ~tc_3000_500_intrbits[dev];
	*(volatile uint32_t *)TC_3000_500_IMR_WRITE = tc_3000_500_imask;
	tc_mb();
}

void
tc_3000_500_intr_disestablish(device_t tcadev, void *cookie)
{
	u_long dev = (u_long)cookie;

#ifdef DIAGNOSTIC
	/* XXX bounds-check cookie. */
#endif

	if (tc_3000_500_intr[dev].tci_func == tc_3000_500_intrnull)
		panic("tc_3000_500_intr_disestablish: cookie %lu bad intr",
		    dev);

	tc_3000_500_imask |= tc_3000_500_intrbits[dev];
	*(volatile uint32_t *)TC_3000_500_IMR_WRITE = tc_3000_500_imask;
	tc_mb();

	tc_3000_500_intr[dev].tci_func = tc_3000_500_intrnull;
	tc_3000_500_intr[dev].tci_arg = (void *)dev;
}

int
tc_3000_500_intrnull(void *val)
{

	panic("tc_3000_500_intrnull: uncaught TC intr for cookie %ld",
	    (u_long)val);
}

void
tc_3000_500_iointr(void *arg, unsigned long vec)
{
	uint32_t ir;
	int ifound;

#ifdef DIAGNOSTIC
	int s;
	if (vec != 0x800)
		panic("INVALID ASSUMPTION: vec 0x%lx, not 0x800", vec);
	s = splhigh();
	if (s != ALPHA_PSL_IPL_IO)
		panic("INVALID ASSUMPTION: IPL %d, not %d", s,
		    ALPHA_PSL_IPL_IO);
	splx(s);
#endif

	do {
		tc_syncbus();
		ir = *(volatile uint32_t *)TC_3000_500_IR_CLEAR;

		/* Ignore interrupts that we haven't enabled. */
		ir &= ~(tc_3000_500_imask & 0x1ff);

		ifound = 0;

#define	INCRINTRCNT(slot)	tc_3000_500_intr[slot].tci_evcnt.ev_count++

#define	CHECKINTR(slot)							\
		if (ir & tc_3000_500_intrbits[slot]) {			\
			ifound = 1;					\
			INCRINTRCNT(slot);				\
			(*tc_3000_500_intr[slot].tci_func)		\
			    (tc_3000_500_intr[slot].tci_arg);		\
		}
		/* Do them in order of priority; highest slot # first. */
		CHECKINTR(TC_3000_500_DEV_CXTURBO);
		CHECKINTR(TC_3000_500_DEV_IOASIC);
		CHECKINTR(TC_3000_500_DEV_TCDS);
		CHECKINTR(TC_3000_500_DEV_OPT5);
		CHECKINTR(TC_3000_500_DEV_OPT4);
		CHECKINTR(TC_3000_500_DEV_OPT3);
		CHECKINTR(TC_3000_500_DEV_OPT2);
		CHECKINTR(TC_3000_500_DEV_OPT1);
		CHECKINTR(TC_3000_500_DEV_OPT0);
#undef CHECKINTR

#ifdef DIAGNOSTIC
#define PRINTINTR(msg, bits)						\
	if (ir & bits)							\
		printf(msg);
		PRINTINTR("Second error occurred\n", TC_3000_500_IR_ERR2);
		PRINTINTR("DMA buffer error\n", TC_3000_500_IR_DMABE);
		PRINTINTR("DMA cross 2K boundary\n", TC_3000_500_IR_DMA2K);
		PRINTINTR("TC reset in progress\n", TC_3000_500_IR_TCRESET);
		PRINTINTR("TC parity error\n", TC_3000_500_IR_TCPAR);
		PRINTINTR("DMA tag error\n", TC_3000_500_IR_DMATAG);
		PRINTINTR("Single-bit error\n", TC_3000_500_IR_DMASBE);
		PRINTINTR("Double-bit error\n", TC_3000_500_IR_DMADBE);
		PRINTINTR("TC I/O timeout\n", TC_3000_500_IR_TCTIMEOUT);
		PRINTINTR("DMA block too long\n", TC_3000_500_IR_DMABLOCK);
		PRINTINTR("Invalid I/O address\n", TC_3000_500_IR_IOADDR);
		PRINTINTR("DMA scatter/gather invalid\n", TC_3000_500_IR_DMASG);
		PRINTINTR("Scatter/gather parity error\n",
		    TC_3000_500_IR_SGPAR);
#undef PRINTINTR
#endif
	} while (ifound);
}

#if NWSDISPLAY > 0
/*
 * tc_3000_500_fb_cnattach --
 *	Attempt to map the CTB output device to a slot and attach the
 * framebuffer as the output side of the console.
 */
int
tc_3000_500_fb_cnattach(uint64_t turbo_slot)
{
	uint32_t output_slot;

	output_slot = turbo_slot & 0xffffffff;

	if (output_slot >= tc_3000_500_nslots) {
		return EINVAL;
	}

	if (hwrpb->rpb_variation & SV_GRAPHICS) {
		if (output_slot == 0) {
#if NSFB > 0
			sfb_cnattach(KV(0x1e0000000) + 0x02000000);
			return 0;
#else
			return ENXIO;
#endif
		}
	} else {
		/*
		 * Slots 0-2 in the tc_3000_500_slots array are only
		 * on the 500 models that also have the CXTurbo
		 * (500/800/900) and a total of 6 TC slots.  For the
		 * 400/600/700, slots 0-2 are in table locations 3-5, so
		 * offset the CTB slot by 3 to get the address in our table.
		 */
		output_slot += 3;
	}
	return tc_fb_cnattach(tc_3000_500_slots[output_slot-1].tcs_addr);
}
#endif /* NWSDISPLAY */

#if 0
/*
 * tc_3000_500_ioslot --
 *	Set the PBS bits for devices on the TC.
 */
void
tc_3000_500_ioslot(uint32_t slot, uint32_t flags, int set)
{
	volatile uint32_t *iosp;
	uint32_t ios;
	int s;
	
	iosp = (volatile uint32_t *)TC_3000_500_IOSLOT;
	ios = *iosp;
	flags <<= (slot * 3);
	if (set)
		ios |= flags;
	else
		ios &= ~flags;
	s = splhigh();
	*iosp = ios;
	tc_mb();
	splx(s);
}
#endif
