/*	$NetBSD: algor_p5064_intr.c,v 1.1 2001/05/28 16:22:14 thorpej Exp $	*/

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
 * Platform-specific interrupt support for the Algorithmics P-5064.
 *
 * The Algorithmics P-5064 has an interrupt controller that is pretty
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

#include <algor/algor/algor_p5064reg.h>
#include <algor/algor/algor_p5064var.h>

#include <algor/algor/clockvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/isa/isavar.h>

#define	REGVAL(x)	*((__volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

#define	IRQREG_LOCINT		0
#define	IRQREG_PANIC		1
#define	IRQREG_PCIINT		2
#define	IRQREG_ISAINT		3
#define	IRQREG_KBDINT		4
#define	NIRQREG			5

struct irqreg {
	bus_addr_t	addr;
	u_int32_t	val;
};

struct irqmap {
	struct evcnt	*irqcount;
	int		irqreg;
	u_int8_t	irqbit;
	int		xbarreg;
	int		xbarshift;
};

struct irqreg p5064_irqregs[NIRQREG] = {
	{ P5064_LOCINT,		0 },
	{ P5064_PANIC,		0 },
	{ P5064_PCIINT,		0 },
	{ P5064_ISAINT,		0 },
	{ P5064_KBDINT,		0 },
};

#define	NSTEERREG		5

struct irqreg p5064_irqsteer[NSTEERREG] = {
	{ P5064_XBAR0,		0 },
	{ P5064_XBAR1,		0 },
	{ P5064_XBAR2,		0 },
	{ P5064_XBAR3,		0 },
	{ P5064_XBAR4,		0 },
};

struct intrhand {
	LIST_ENTRY(intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	const struct irqmap *ih_irqmap;
};

struct intrhead {
	LIST_HEAD(, intrhand) intr_q;
	struct evcnt intr_count;
};

#define	NINTRS		4	/* MIPS INT0 - INT3 */

struct intrhead p5064_intrtab[NINTRS];
const char *p5064_intrnames[NINTRS] = {
	"int 0",
	"int 1",
	"int 2",
	"int 3 (clock)",
};

#define	IRQ_ETHERNET	4
#define	IRQ_SCSI	5
#define	IRQ_USB		6
#define	IRQ_MAX		6

const char *p5064_pci_intrnames[IRQ_MAX + 1] = {
	"PCIIRQ 0",
	"PCIIRQ 1",
	"PCIIRQ 2",
	"PCIIRQ 3",
	"Ethernet",
	"SCSI",
	"USB",
};

struct evcnt p5064_pci_intrcount[IRQ_MAX + 1];

const struct irqmap p5064_pci_irqmap[IRQ_MAX + 1] = {
	/* PCIIRQ 0 */
	{ &p5064_pci_intrcount[0],
	  IRQREG_PCIINT,	PCIINT_PCI0,
	  2,			0 },

	/* PCIIRQ 1 */
	{ &p5064_pci_intrcount[1],
	  IRQREG_PCIINT,	PCIINT_PCI1,
	  2,			2 },

	/* PCIIRQ 2 */
	{ &p5064_pci_intrcount[2],
	  IRQREG_PCIINT,	PCIINT_PCI2,
	  2,			4 },

	/* PCIIRQ 3 */
	{ &p5064_pci_intrcount[3],
	  IRQREG_PCIINT,	PCIINT_PCI3,
	  2,			6 },

	/* Ethernet */
	{ &p5064_pci_intrcount[4],
	  IRQREG_PCIINT,	PCIINT_ETH,
	  4,			2 },

	/* SCSI */
	{ &p5064_pci_intrcount[5],
	  IRQREG_PCIINT,	PCIINT_SCSI,
	  4,			4 },

	/* USB */
	{ &p5064_pci_intrcount[6],
	  IRQREG_PCIINT,	PCIINT_USB,
	  4,			6 },
};

const char *p5064_isa_intrnames[16] = {
	"irq 0",
	"mkbd kbd",
	"irq 2",
	"com 2",
	"com 1",
	"irq 5",
	"floppy",
	"centronics",
	"rtc",
	"irq 9",
	"irq 10",
	"irq 11",
	"mkbd mouse",
	"irq 13",
	"ide primary",
	"ide secondary"
};

struct evcnt p5064_isa_intrcount[16];

const struct irqmap p5064_isa_irqmap[16] = {
	/* IRQ 0 */
	{ &p5064_isa_intrcount[0],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 1 -- keyboard */
	{ &p5064_isa_intrcount[1],
	  IRQREG_LOCINT,	LOCINT_MKBD,
	  0,			4 },

	/* IRQ 2 */
	{ &p5064_isa_intrcount[2],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 3 -- COM2 */
	{ &p5064_isa_intrcount[3],
	  IRQREG_LOCINT,	LOCINT_COM2,
	  1,			0 },

	/* IRQ 4 -- COM1 */
	{ &p5064_isa_intrcount[4],
	  IRQREG_LOCINT,	LOCINT_COM1,
	  0,			6 },

	/* IRQ 5 */
	{ &p5064_isa_intrcount[5],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 6 -- floppy controller */
	{ &p5064_isa_intrcount[6],
	  IRQREG_LOCINT,	LOCINT_FLP,
	  0,			2 },

	/* IRQ 7 -- parallel port */
	{ &p5064_isa_intrcount[7],
	  IRQREG_LOCINT,	LOCINT_CENT,
	  1,			2 },

	/* IRQ 8 -- RTC */
	{ &p5064_isa_intrcount[8],
	  IRQREG_LOCINT,	LOCINT_RTC,
	  1,			6 },

	/* IRQ 9 */
	{ &p5064_isa_intrcount[9],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 10 */
	{ &p5064_isa_intrcount[10],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 11 */
	{ &p5064_isa_intrcount[11],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 12 -- mouse */
	{ &p5064_isa_intrcount[12],
	  IRQREG_LOCINT,	LOCINT_MKBD,
	  0,			4 },

	/* IRQ 13 */
	{ &p5064_isa_intrcount[13],
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IRQ 14 -- IDE 0 */
	{ &p5064_isa_intrcount[14],
	  IRQREG_ISAINT,	ISAINT_IDE0,
	  3,			2 },

	/* IRQ 15 -- IDE 1 */
	{ &p5064_isa_intrcount[15],
	  IRQREG_ISAINT,	ISAINT_IDE1,
	  3,			4 },
};

void	algor_p5064_intr_disestablish(void *, void *);
void	*algor_p5064_intr_establish(const struct irqmap *, int,
	    int (*)(void *), void *);

int	algor_p5064_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *algor_p5064_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *algor_p5064_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*algor_p5064_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	*algor_p5064_pciide_compat_intr_establish(void *, struct device *,
	    struct pci_attach_args *, int, int (*)(void *), void *);
void	algor_p5064_pci_conf_interrupt(void *, int, int, int, int, int *);

const struct evcnt *algor_p5064_isa_intr_evcnt(void *, int);
void	*algor_p5064_isa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
int	algor_p5064_isa_intr_alloc(void *, int, int, int *);

void	algor_p5064_init_clock_intr(void);
void	algor_p5064_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void
algor_p5064_intr_init(struct p5064_config *acp)
{
	int i;

	for (i = 0; i < NIRQREG; i++)
		REGVAL(p5064_irqregs[i].addr) = p5064_irqregs[i].val;

	for (i = 0; i < NSTEERREG; i++)
		REGVAL(p5064_irqsteer[i].addr) = p5064_irqsteer[i].val;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&p5064_intrtab[i].intr_q);
		evcnt_attach_dynamic(&p5064_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", p5064_intrnames[i]);
	}

	for (i = 0; i <= IRQ_MAX; i++)
		evcnt_attach_dynamic(&p5064_pci_intrcount[i],
		    EVCNT_TYPE_INTR, NULL, "pci", p5064_pci_intrnames[i]);

	for (i = 0; i < 16; i++)
		evcnt_attach_dynamic(&p5064_isa_intrcount[i],
		    EVCNT_TYPE_INTR, NULL, "isa", p5064_isa_intrnames[i]);

	acp->ac_pc.pc_intr_v = NULL;
	acp->ac_pc.pc_intr_map = algor_p5064_pci_intr_map;
	acp->ac_pc.pc_intr_string = algor_p5064_pci_intr_string;
	acp->ac_pc.pc_intr_evcnt = algor_p5064_pci_intr_evcnt;
	acp->ac_pc.pc_intr_establish = algor_p5064_pci_intr_establish;
	acp->ac_pc.pc_intr_disestablish = algor_p5064_intr_disestablish;
	acp->ac_pc.pc_conf_interrupt = algor_p5064_pci_conf_interrupt;
	acp->ac_pc.pc_pciide_compat_intr_establish =
	    algor_p5064_pciide_compat_intr_establish;

	acp->ac_ic.ic_v = NULL;
	acp->ac_ic.ic_intr_evcnt = algor_p5064_isa_intr_evcnt;
	acp->ac_ic.ic_intr_establish = algor_p5064_isa_intr_establish;
	acp->ac_ic.ic_intr_disestablish = algor_p5064_intr_disestablish;
	acp->ac_ic.ic_intr_alloc = algor_p5064_isa_intr_alloc;

	algor_iointr = algor_p5064_iointr;
	algor_init_clock_intr = algor_p5064_init_clock_intr;
}

void *
algor_p5064_intr_establish(const struct irqmap *irqmap, int level,
    int (*func)(void *), void *arg)
{
	struct intrhand *ih;
	int s;

	/*
	 * XXX SHARING OF INTERRUPTS OF DIFFERENT LEVELS ON THE
	 * XXX SAME PCIIRQ OR ISA BRIDGE IRQ IS COMPLETELY BROKEN.
	 * XXX FIXME!!!!
	 */

	/*
	 * XXX REFERENCE COUNTING OF PCIIRQS ISA BRIDGE IRQS
	 * XXX IS COMPLETLY BROKEN, MAKING IT IMPOSSIBLE TO
	 * XXX IMPLEMENT INTR_DISESTABLISH.
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
		printf("algor_p5064_intr_establish: bad level %d\n", level);
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
	LIST_INSERT_HEAD(&p5064_intrtab[level].intr_q, ih, ih_q);

	/*
	 * Then, steer the interrupt to the intended MIPS interrupt line.
	 */
	p5064_irqsteer[irqmap->xbarreg].val &= ~(3 << irqmap->xbarshift);
	p5064_irqsteer[irqmap->xbarreg].val |= (level << irqmap->xbarshift);
	REGVAL(p5064_irqsteer[irqmap->xbarreg].addr) =
	    p5064_irqsteer[irqmap->xbarreg].val;

	/*
	 * Now enable it.
	 */
	/* XXX XXX XXX */
	if (p5064_irqregs[irqmap->irqreg].val & irqmap->irqbit)
		printf("WARNING: IRQREG %d BIT 0x%02x ALREADY IN USE!\n",
		    irqmap->irqreg, irqmap->irqbit);
	/* XXX XXX XXX */
	p5064_irqregs[irqmap->irqreg].val |= irqmap->irqbit;
	REGVAL(p5064_irqregs[irqmap->irqreg].addr) =
	    p5064_irqregs[irqmap->irqreg].val;

	splx(s);

	return (ih);
}

void
algor_p5064_intr_disestablish(void *v, void *cookie)
{

	panic("algor_p5064_intr_disestablish: I'm totally broken.");
}

void
algor_p5064_init_clock_intr(void)
{

	/* Steer the clock interrupt to MIPS INT3 */
	p5064_irqsteer[1].val |= (3 << 6);
	REGVAL(p5064_irqsteer[1].addr) = p5064_irqsteer[1].val;

	/* Now enable it. */
	p5064_irqregs[IRQREG_LOCINT].val |= LOCINT_RTC;
	REGVAL(p5064_irqregs[IRQREG_LOCINT].addr) =
	    p5064_irqregs[IRQREG_LOCINT].val;
}

void
algor_p5064_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	struct intrhand *ih;
	int level, i;
	u_int32_t irr[NIRQREG];

	/* Check for PANIC interrupts. */
	if (ipending & MIPS_INT_MASK_4) {
		irr[IRQREG_PANIC] = REGVAL(p5064_irqregs[IRQREG_PANIC].addr);
		if (irr[IRQREG_PANIC] & PANIC_IOPERR)
			printf("WARNING: I/O parity error\n");
		if (irr[IRQREG_PANIC] & PANIC_ISANMI)
			printf("WARNING: ISA NMI\n");
		if (irr[IRQREG_PANIC] & PANIC_BERR)
			printf("WARNING: Bus error\n");
		if (irr[IRQREG_PANIC] & PANIC_PFAIL)
			printf("WARNING: Power failure\n");
		if (irr[IRQREG_PANIC] & PANIC_DEBUG) {
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
		REGVAL(p5064_irqregs[IRQREG_PANIC].addr) = irr[IRQREG_PANIC];
	}

	/* Do clock interrupts. */
	if (ipending & MIPS_INT_MASK_3) {
		struct clockframe cf;

		(*clockfns->cf_intrack)(clockdev);

		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);

		p5064_intrtab[3].intr_count.ev_count++;

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
		if (i == IRQREG_PANIC)
			continue;
		irr[i] = REGVAL(p5064_irqregs[i].addr) & p5064_irqregs[i].val;
	}

	for (level = 2; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;
		p5064_intrtab[level].intr_count.ev_count++;
		for (ih = LIST_FIRST(&p5064_intrtab[level].intr_q);
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
algor_p5064_pci_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	static const int pciirqmap[6/*device*/][4/*pin*/] = {
		{ IRQ_ETHERNET, -1, -1, -1 },	/* 0: Ethernet */
		{ IRQ_SCSI, -1, -1, -1 },	/* 1: SCSI */
		{ -1, -1, -1, IRQ_USB },	/* 2: PCI-ISA bridge */
		{ 0, 1, 2, 3 },			/* 3: PCI slot 3 */
		{ 3, 0, 1, 2 },			/* 4: PCI slot 2 */
		{ 2, 3, 0, 1 },			/* 5: PCI slot 1 */
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
		printf("algor_p5064_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (device > 5) {
		printf("algor_p5064_pci_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	irq = pciirqmap[device][buspin - 1];
	if (irq == -1) {
		printf("algor_p5064_pci_intr_map: no mapping for "
		    "device %d pin %d\n", device, buspin);
		return (1);
	}

	*ihp = irq;
	return (0);
}

const char *
algor_p5064_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	if (ih > IRQ_MAX)
		panic("algor_p5064_intr_string: bogus IRQ %ld\n", ih);

	return (p5064_pci_intrnames[ih]);
}

const struct evcnt *
algor_p5064_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX */
	return (NULL);
}

void *
algor_p5064_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih > IRQ_MAX)
		panic("algor_p5064_intr_establish: bogus IRQ %ld\n", ih);

	return (algor_p5064_intr_establish(&p5064_pci_irqmap[ih],
	    level, func, arg));
}

void
algor_p5064_pci_conf_interrupt(void *v, int bus, int dev, int func, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}

void *
algor_p5064_pciide_compat_intr_establish(void *v, struct device *dev,
    struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc; 
	void *cookie;
	int bus, irq;

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	irq = PCIIDE_COMPAT_IRQ(chan);
	cookie = algor_p5064_isa_intr_establish(NULL /*XXX*/, irq, IST_EDGE,
	    IPL_BIO, func, arg);
	if (cookie == NULL)
		return (NULL);
	printf("%s: %s channel interrupting at ISA IRQ %d\n", dev->dv_xname,
	    PCIIDE_CHANNEL_NAME(chan), irq);
	return (cookie);
}

/*****************************************************************************
 * ISA interrupt support
 *****************************************************************************/

const struct evcnt *
algor_p5064_isa_intr_evcnt(void *v, int iirq)
{

	/* XXX */
	return (NULL);
}

void *
algor_p5064_isa_intr_establish(void *v, int iirq, int type, int level,
    int (*func)(void *), void *arg)
{

	if (iirq > 15 || type == IST_NONE)
		panic("algor_p5064_isa_intr_establish: bad irq or type");

	/*
	 * XXX WE CURRENTLY DO NOT DO ANYTHING USEFUL AT ALL WITH
	 * XXX REGULAR ISA INTERRUPTS.
	 */

	return (algor_p5064_intr_establish(&p5064_isa_irqmap[iirq],
	    level, func, arg));
}

int
algor_p5064_isa_intr_alloc(void *v, int mask, int type, int *iirq)
{

	/* XXX */
	return (1);
}
