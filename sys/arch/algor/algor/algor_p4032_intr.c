/*	$NetBSD: algor_p4032_intr.c,v 1.1 2001/06/01 16:00:03 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Platform-specific interrupt support for the Algorithmics P-4032.
 *
 * The Algorithmics P-4032 has an interrupt controller that is pretty
 * flexible -- it can take an interrupt source and route it to an
 * arbitrary MIPS CPU hardware interrupt pin.
 *
 * Unfortunately, there aren't enough source pins as inputs to the
 * controller.  It makes it difficult to share interrupts between
 * devices that have different logical levels (BIO, NET, etc.).
 *
 * As a result, this code needs a lot of work -- needs to have the
 * interrupts fully-virtualized -- I just wanted to get *something*
 * working quickly.  --thorpej@netbsd.org
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <algor/algor/algor_p4032reg.h>
#include <algor/algor/algor_p4032var.h>

#include <algor/algor/clockvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/isa/isavar.h>

#define	REGVAL(x)	*((__volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

#define	IRQREG_8BIT		0
#define	IRQREG_ERROR		1
#define	IRQREG_PCI		2
#define	NIRQREG			3

struct irqreg {
	bus_addr_t	addr;
	u_int32_t	val;
};

struct irqreg p4032_irqregs[NIRQREG] = {
	{ P4032_IRR0,		0 },
	{ P4032_IRR1,		0 },
	{ P4032_IRR2,		0 },
};

#define	NSTEERREG		3

struct irqreg p4032_irqsteer[NSTEERREG] = {
	{ P4032_XBAR0,		0 },
	{ P4032_XBAR1,		0 },
	{ P4032_XBAR2,		0 },
};

struct intrhand {
	LIST_ENTRY(intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	const struct p4032_irqmap *ih_irqmap;
};

struct intrhead {
	LIST_HEAD(, intrhand) intr_q;
	struct evcnt intr_count;
};

#define	NINTRS		4	/* MIPS INT0 - INT3 */

struct intrhead p4032_intrtab[NINTRS];
const char *p4032_intrnames[NINTRS] = {
	"int 0",
	"int 1",
	"int 2",
	"int 3 (clock)",
};

const char *p4032_pci_intrnames[4] = {
	"PCIIRQ 0",
	"PCIIRQ 1",
	"PCIIRQ 2",
	"PCIIRQ 3",
};

struct evcnt p4032_pci_intrcount[4];

const struct p4032_irqmap p4032_pci_irqmap[4] = {
	/* PCIIRQ 0 */
	{ &p4032_pci_intrcount[0],
	  IRQREG_PCI,		IRR2_PCIIRQ0,
	  2,			0 },

	/* PCIIRQ 1 */
	{ &p4032_pci_intrcount[1],
	  IRQREG_PCI,		IRR2_PCIIRQ1,
	  2,			2 },

	/* PCIIRQ 2 */
	{ &p4032_pci_intrcount[2],
	  IRQREG_PCI,		IRR2_PCIIRQ2,
	  2,			4 },

	/* PCIIRQ 3 */
	{ &p4032_pci_intrcount[3],
	  IRQREG_PCI,		IRR2_PCIIRQ3,
	  2,			6 },
};

const char *p4032_8bit_intrnames[8] = {
	"PCI ctlr",
	"floppy",
	"pckbc",
	"com 1",
	"com 2",
	"centronics",
	"gpio",
	"rtc",
};

struct evcnt p4032_8bit_intrcount[8];

const struct p4032_irqmap p4032_8bit_irqmap[8] = {
	{ &p4032_8bit_intrcount[0],
	  IRQREG_8BIT,		IRR0_PCICTLR,
	  0,			0 },

	{ &p4032_8bit_intrcount[1],
	  IRQREG_8BIT,		IRR0_FLOPPY,
	  0,			2 },

	{ &p4032_8bit_intrcount[2],
	  IRQREG_8BIT,		IRR0_PCKBC,
	  0,			4 },

	{ &p4032_8bit_intrcount[3],
	  IRQREG_8BIT,		IRR0_COM1,
	  0,			6 },

	{ &p4032_8bit_intrcount[4],
	  IRQREG_8BIT,		IRR0_COM2,
	  1,			0 },

	{ &p4032_8bit_intrcount[5],
	  IRQREG_8BIT,		IRR0_LPT,
	  1,			2 },

	{ &p4032_8bit_intrcount[6],
	  IRQREG_8BIT,		IRR0_GPIO,
	  1,			4 },

	{ &p4032_8bit_intrcount[7],
	  IRQREG_8BIT,		IRR0_RTC,
	  1,			6 },
};

int	algor_p4032_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *algor_p4032_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *algor_p4032_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*algor_p4032_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	algor_p4032_pci_conf_interrupt(void *, int, int, int, int, int *);

void	algor_p4032_init_clock_intr(void);
void	algor_p4032_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void
algor_p4032_intr_init(struct p4032_config *acp)
{
	int i;

	for (i = 0; i < NIRQREG; i++)
		REGVAL(p4032_irqregs[i].addr) = p4032_irqregs[i].val;

	for (i = 0; i < NSTEERREG; i++)
		REGVAL(p4032_irqsteer[i].addr) = p4032_irqsteer[i].val;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&p4032_intrtab[i].intr_q);
		evcnt_attach_dynamic(&p4032_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", p4032_intrnames[i]);
	}

	for (i = 0; i < 4; i++)
		evcnt_attach_dynamic(&p4032_pci_intrcount[i],
		    EVCNT_TYPE_INTR, NULL, "pci", p4032_pci_intrnames[i]);

	for (i = 0; i < 8; i++)
		evcnt_attach_dynamic(&p4032_8bit_intrcount[i],
		    EVCNT_TYPE_INTR, NULL, "8bit", p4032_8bit_intrnames[i]);

	acp->ac_pc.pc_intr_v = NULL;
	acp->ac_pc.pc_intr_map = algor_p4032_pci_intr_map;
	acp->ac_pc.pc_intr_string = algor_p4032_pci_intr_string;
	acp->ac_pc.pc_intr_evcnt = algor_p4032_pci_intr_evcnt;
	acp->ac_pc.pc_intr_establish = algor_p4032_pci_intr_establish;
	acp->ac_pc.pc_intr_disestablish = algor_p4032_intr_disestablish;
	acp->ac_pc.pc_conf_interrupt = algor_p4032_pci_conf_interrupt;
	acp->ac_pc.pc_pciide_compat_intr_establish = NULL;

	algor_iointr = algor_p4032_iointr;
	algor_init_clock_intr = algor_p4032_init_clock_intr;
}

void *
algor_p4032_intr_establish(const struct p4032_irqmap *irqmap, int level,
    int (*func)(void *), void *arg)
{
	struct intrhand *ih;
	int s;

	/*
	 * XXX SHARING OF INTERRUPTS OF DIFFERENT LEVELS ON THE
	 * XXX SAME PCIIRQ IS COMPLETELY BROKEN.
	 * XXX FIXME!!!!
	 */

	/*
	 * XXX REFERENCE COUNTING OF PCIIRQS IS COMPLETELY
	 * XXX BROKEN, MAKING IT IMPOSSIBLE TO IMPLEMENT
	 * XXX INTR_DISESTABLISH.
	 * XXX FIXME!!!!
	 */

	switch (level) {
	case IPL_BIO:
		level = 0;
		break;

	case IPL_NET:
		level = 1;
		break;

	case IPL_TTY:
#if 0
	case IPL_SERIAL:	/* same as IPL_TTY */
#endif
		level = 2;
		break;

	case IPL_CLOCK:
		level = 3;
		break;

	default:
		printf("algor_p4032_intr_establish: bad level %d\n", level);
		return (NULL);
	}

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irqmap = irqmap;
 
	s = splhigh();

	/*
	 * First, link it into the interrupt table.
	 */
	LIST_INSERT_HEAD(&p4032_intrtab[level].intr_q, ih, ih_q);

	/*
	 * Then, steer the interrupt to the intended MIPS interrupt line.
	 */
	p4032_irqsteer[irqmap->xbarreg].val &= ~(3 << irqmap->xbarshift);
	p4032_irqsteer[irqmap->xbarreg].val |= (level << irqmap->xbarshift);
	REGVAL(p4032_irqsteer[irqmap->xbarreg].addr) =
	    p4032_irqsteer[irqmap->xbarreg].val;

	/*
	 * Now enable it.
	 */
	/* XXX XXX XXX */
	if (p4032_irqregs[irqmap->irqreg].val & irqmap->irqbit)
		printf("WARNING: IRQREG %d BIT 0x%02x ALREADY IN USE!\n",
		    irqmap->irqreg, irqmap->irqbit);
	/* XXX XXX XXX */
	p4032_irqregs[irqmap->irqreg].val |= irqmap->irqbit;
	REGVAL(p4032_irqregs[irqmap->irqreg].addr) =
	    p4032_irqregs[irqmap->irqreg].val;

	splx(s);

	return (ih);
}

void
algor_p4032_intr_disestablish(void *v, void *cookie)
{

	panic("algor_p4032_intr_disestablish: I'm totally broken.");
}

void
algor_p4032_init_clock_intr(void)
{

	/* Steer the clock interrupt to MIPS INT3 */
	p4032_irqsteer[1].val |= (3 << 6);
	REGVAL(p4032_irqsteer[1].addr) = p4032_irqsteer[1].val;

	/* Now enable it. */
	p4032_irqregs[IRQREG_8BIT].val |= IRR0_RTC;
	REGVAL(p4032_irqregs[IRQREG_8BIT].addr) =
	    p4032_irqregs[IRQREG_8BIT].val;
}

void
algor_p4032_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	struct intrhand *ih;
	int level, i;
	u_int32_t irr[NIRQREG];

	/* Check for ERROR interrupts. */
	if (ipending & MIPS_INT_MASK_4) {
		irr[IRQREG_ERROR] = REGVAL(p4032_irqregs[IRQREG_ERROR].addr);
		if (irr[IRQREG_ERROR] & IRR1_BUSERR)
			printf("WARNING: Bus error\n");
		if (irr[IRQREG_ERROR] & IRR1_POWERFAIL)
			printf("WARNING: Power failure\n");
		if (irr[IRQREG_ERROR] & IRR1_DEBUG) {
#ifdef DDB
			printf("Debug switch -- entering debugger\n");
			led_display('D','D','B',' ');
			Debugger();
			led_display('N','B','S','D');
#else
			printf("Debug switch ignored -- "
			    "no debugger configured\n");
#endif
		}

		/* Clear them. */
		REGVAL(p4032_irqregs[IRQREG_ERROR].addr) = irr[IRQREG_ERROR];
	}

	/* Do clock interrupts. */
	if (ipending & MIPS_INT_MASK_3) {
		struct clockframe cf;

		/*
		 * XXX Hi, um, yah, we need to deal with
		 * XXX the floppy interrupt here, too.
		 */

		(*clockfns->cf_intrack)(clockdev);

		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);

		p4032_intrtab[3].intr_count.ev_count++;
		p4032_8bit_intrcount[7].ev_count++;

		/* Re-enable clock interrupts. */
		cause &= ~MIPS_INT_MASK_3;
		_splset(MIPS_SR_INT_IE | MIPS_INT_MASK_3);
	}

	/*
	 * Read the interrupt pending registers, mask them with the
	 * ones we have enabled, and service them in order of decreasing
	 * priority.
	 */
	for (i = 0; i < NIRQREG; i++) {
		if (i == IRQREG_ERROR)
			continue;
		irr[i] = REGVAL(p4032_irqregs[i].addr) & p4032_irqregs[i].val;
	}

	for (level = 2; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;
		p4032_intrtab[level].intr_count.ev_count++;
		for (ih = LIST_FIRST(&p4032_intrtab[level].intr_q);
		     ih != NULL; ih = LIST_NEXT(ih, ih_q)) {
			if (irr[ih->ih_irqmap->irqreg] &
			    ih->ih_irqmap->irqbit) {
				ih->ih_irqmap->irqcount->ev_count++;
				(*ih->ih_func)(ih->ih_arg);
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << level);
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}

/*****************************************************************************
 * PCI interrupt support
 *****************************************************************************/

int
algor_p4032_pci_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	static const int pciirqmap[6/*device*/][4/*pin*/] = {
		{ 1, -1, -1, -1 },		/* 5: Ethernet */
		{ 2, 3, 0, 1 },			/* 6: PCI slot 1 */
		{ 3, 0, 1, 2 },			/* 7: PCI slot 2 */
		{ 0, -1, -1, -1 },		/* 8: SCSI */
		{ -1, -1, -1, -1 },		/* 9: not used */
		{ 0, 1, 2, 3 },			/* 10: custom connector */
	};
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("algor_p4032_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (device < 5 || device > 10) {
		printf("algor_p4032_pci_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	irq = pciirqmap[device - 5][buspin - 1];
	if (irq == -1) {
		printf("algor_p4032_pci_intr_map: no mapping for "
		    "device %d pin %d\n", device, buspin);
		return (1);
	}

	*ihp = irq;
	return (0);
}

const char *
algor_p4032_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	if (ih > 3)
		panic("algor_p4032_intr_string: bogus IRQ %ld\n", ih);

	return (p4032_pci_intrnames[ih]);
}

const struct evcnt *
algor_p4032_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX */
	return (NULL);
}

void *
algor_p4032_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih > 3)
		panic("algor_p4032_intr_establish: bogus IRQ %ld\n", ih);

	return (algor_p4032_intr_establish(&p4032_pci_irqmap[ih],
	    level, func, arg));
}

void
algor_p4032_pci_conf_interrupt(void *v, int bus, int dev, int func, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}
