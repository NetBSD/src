/*	$NetBSD: algor_p5064_intr.c,v 1.7.2.1 2001/08/25 06:14:58 thorpej Exp $	*/

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
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <mips/locore.h>

#include <dev/ic/mc146818reg.h>

#include <algor/algor/algor_p5064reg.h>
#include <algor/algor/algor_p5064var.h>

#include <algor/algor/clockvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/isa/isavar.h>

#define	REGVAL(x)	*((__volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

struct p5064_irqreg {
	bus_addr_t	addr;
	u_int32_t	val;
};

#define	IRQREG_LOCINT		0
#define	IRQREG_PANIC		1
#define	IRQREG_PCIINT		2
#define	IRQREG_ISAINT		3
#define	IRQREG_KBDINT		4
#define	NIRQREG			5

struct p5064_irqreg p5064_irqregs[NIRQREG] = {
	{ P5064_LOCINT,		0 },
	{ P5064_PANIC,		0 },
	{ P5064_PCIINT,		0 },
	{ P5064_ISAINT,		0 },
	{ P5064_KBDINT,		0 },
};

#define	NSTEERREG		5

struct p5064_irqreg p5064_irqsteer[NSTEERREG] = {
	{ P5064_XBAR0,		0 },
	{ P5064_XBAR1,		0 },
	{ P5064_XBAR2,		0 },
	{ P5064_XBAR3,		0 },
	{ P5064_XBAR4,		0 },
};

#define	NPCIIRQS		7

#define	NLOCIRQS		6

#define	NISAIRQS		3

#define	IRQMAP_PCIBASE		0
#define	IRQMAP_LOCBASE		NPCIIRQS
#define	IRQMAP_ISABASE		(IRQMAP_LOCBASE + NLOCIRQS)
#define	NIRQMAPS		(IRQMAP_ISABASE + NISAIRQS)

const char *p5064_intrnames[NIRQMAPS] = {
	/*
	 * PCI INTERRUPTS
	 */
	"PCIIRQ 0",
	"PCIIRQ 1",
	"PCIIRQ 2",
	"PCIIRQ 3",
	"Ethernet IRQ",
	"SCSI IRQ",
	"USB IRQ",

	/*
	 * LOCAL INTERRUPTS
	 */
	"mkbd",
	"com 1",
	"com 2",
	"floppy",
	"centronics",
	"mcclock",

	/*
	 * ISA interrupts.
	 */
	"bridge",
	"IDE primary",
	"IDE secondary",
};

struct p5064_irqmap { 
	int	irqidx;
	int	cpuintr;
	int	irqreg;
	int	irqbit;
	int	xbarreg;
	int	xbarshift;
}; 

const struct p5064_irqmap p5064_irqmap[NIRQMAPS] = {
	/*
	 * PCI INTERRUPTS
	 */
	/* PCIIRQ 0 */
	{ 0,			1,
	  IRQREG_PCIINT,	PCIINT_PCI0,
	  2,			0 },

	/* PCIIRQ 1 */
	{ 1,			1,
	  IRQREG_PCIINT,	PCIINT_PCI1,
	  2,			2 },

	/* PCIIRQ 2 */
	{ 2,			1,
	  IRQREG_PCIINT,	PCIINT_PCI2,
	  2,			4 },

	/* PCIIRQ 3 */
	{ 3,			1,
	  IRQREG_PCIINT,	PCIINT_PCI3,
	  2,			6 },

	/* Ethernet */
	{ P5064_IRQ_ETHERNET,	1,
	  IRQREG_PCIINT,	PCIINT_ETH,
	  4,			2 },

	/* SCSI */
	{ P5064_IRQ_SCSI,	1,
	  IRQREG_PCIINT,	PCIINT_SCSI,
	  4,			4 },

	/* USB */
	{ P5064_IRQ_USB,	1,
	  IRQREG_PCIINT,	PCIINT_USB,
	  4,			6 },

	/*
	 * LOCAL INTERRUPTS
	 */
	/* keyboard */
	{ P5064_IRQ_MKBD,	2,
	  IRQREG_LOCINT,	LOCINT_MKBD,
	  0,			4 },

	/* COM1 */
	{ P5064_IRQ_COM1,	2,
	  IRQREG_LOCINT,	LOCINT_COM1,
	  0,			6 },

	/* COM2 */
	{ P5064_IRQ_COM2,	2,
	  IRQREG_LOCINT,	LOCINT_COM2,
	  1,			0 },

	/* floppy controller */
	{ P5064_IRQ_FLOPPY,	2,
	  IRQREG_LOCINT,	LOCINT_FLP,
	  0,			2 },

	/* parallel port */
	{ P5064_IRQ_CENTRONICS,	2,
	  IRQREG_LOCINT,	LOCINT_CENT,
	  1,			2 },

	/* RTC */
	{ P5064_IRQ_RTC,	2,
	  IRQREG_LOCINT,	LOCINT_RTC,
	  1,			6 },

	/*
	 * ISA INTERRUPTS
	 */
	/* ISA bridge */
	{ P5064_IRQ_ISABRIDGE,	0,
	  IRQREG_ISAINT,	ISAINT_ISABR,
	  3,			0 },

	/* IDE 0 */
	{ P5064_IRQ_IDE0,	0,
	  IRQREG_ISAINT,	ISAINT_IDE0,
	  3,			2 },

	/* IDE 1 */
	{ P5064_IRQ_IDE1,	0,
	  IRQREG_ISAINT,	ISAINT_IDE1,
	  3,			4 },
};

const int p5064_isa_to_irqmap[16] = {
	-1,			/* 0 */
	P5064_IRQ_MKBD,		/* 1 */
	-1,			/* 2 */
	P5064_IRQ_COM2,		/* 3 */
	P5064_IRQ_COM1,		/* 4 */
	-1,			/* 5 */
	P5064_IRQ_FLOPPY,	/* 6 */
	P5064_IRQ_CENTRONICS,	/* 7 */
	P5064_IRQ_RTC,		/* 8 */
	-1,			/* 9 */
	-1,			/* 10 */
	-1,			/* 11 */
	P5064_IRQ_MKBD,		/* 12 */
	-1,			/* 13 */
	P5064_IRQ_IDE0,		/* 14 */
	P5064_IRQ_IDE1,		/* 15 */
};

struct p5064_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct p5064_intrhead p5064_intrtab[NIRQMAPS];

#define	NINTRS			3	/* MIPS INT0 - INT2 */

struct p5064_cpuintr {
	LIST_HEAD(, algor_intrhand) cintr_list;
	struct evcnt cintr_count;
};

struct p5064_cpuintr p5064_cpuintrs[NINTRS];
const char *p5064_cpuintrnames[NINTRS] = {
	"int 0 (isa)",
	"int 1 (pci)",
	"int 2 (local)",
};

const char *p5064_intrgroups[NINTRS] = {
	"isa",
	"pci",
	"local",
};

void	*algor_p5064_intr_establish(int, int (*)(void *), void *);
void	algor_p5064_intr_disestablish(void *);

int	algor_p5064_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *algor_p5064_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *algor_p5064_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*algor_p5064_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	algor_p5064_pci_intr_disestablish(void *, void *);
void	*algor_p5064_pciide_compat_intr_establish(void *, struct device *,
	    struct pci_attach_args *, int, int (*)(void *), void *);
void	algor_p5064_pci_conf_interrupt(void *, int, int, int, int, int *);

const struct evcnt *algor_p5064_isa_intr_evcnt(void *, int);
void	*algor_p5064_isa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
void	algor_p5064_isa_intr_disestablish(void *, void *);
int	algor_p5064_isa_intr_alloc(void *, int, int, int *);

void	algor_p5064_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void
algor_p5064_intr_init(struct p5064_config *acp)
{
	const struct p5064_irqmap *irqmap;
	int i;

	for (i = 0; i < NIRQREG; i++)
		REGVAL(p5064_irqregs[i].addr) = p5064_irqregs[i].val;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&p5064_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&p5064_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", p5064_cpuintrnames[i]);
	}
	evcnt_attach_static(&mips_int5_evcnt);

	for (i = 0; i < NIRQMAPS; i++) {
		irqmap = &p5064_irqmap[i];

		p5064_irqsteer[irqmap->xbarreg].val |=
		    irqmap->cpuintr << irqmap->xbarshift;

		evcnt_attach_dynamic(&p5064_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, p5064_intrgroups[irqmap->cpuintr],
		    p5064_intrnames[i]);
	}

	for (i = 0; i < NSTEERREG; i++)
		REGVAL(p5064_irqsteer[i].addr) = p5064_irqsteer[i].val;

	acp->ac_pc.pc_intr_v = NULL;
	acp->ac_pc.pc_intr_map = algor_p5064_pci_intr_map;
	acp->ac_pc.pc_intr_string = algor_p5064_pci_intr_string;
	acp->ac_pc.pc_intr_evcnt = algor_p5064_pci_intr_evcnt;
	acp->ac_pc.pc_intr_establish = algor_p5064_pci_intr_establish;
	acp->ac_pc.pc_intr_disestablish = algor_p5064_pci_intr_disestablish;
	acp->ac_pc.pc_conf_interrupt = algor_p5064_pci_conf_interrupt;
	acp->ac_pc.pc_pciide_compat_intr_establish =
	    algor_p5064_pciide_compat_intr_establish;

	acp->ac_ic.ic_v = NULL;
	acp->ac_ic.ic_intr_evcnt = algor_p5064_isa_intr_evcnt;
	acp->ac_ic.ic_intr_establish = algor_p5064_isa_intr_establish;
	acp->ac_ic.ic_intr_disestablish = algor_p5064_isa_intr_disestablish;
	acp->ac_ic.ic_intr_alloc = algor_p5064_isa_intr_alloc;

	algor_intr_establish = algor_p5064_intr_establish;
	algor_intr_disestablish = algor_p5064_intr_disestablish;
	algor_iointr = algor_p5064_iointr;
}

void
algor_p5064_cal_timer(bus_space_tag_t st, bus_space_handle_t sh)
{
	u_long ctrdiff[4], startctr, endctr, cps;
	u_int32_t irr;
	int i;

	/* Disable interrupts first. */
	bus_space_write_1(st, sh, 0, MC_REGB);
	bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	/* Initialize for 16Hz. */
	bus_space_write_1(st, sh, 0, MC_REGA);
	bus_space_write_1(st, sh, 1, MC_BASE_32_KHz | MC_RATE_16_Hz);

	REGVAL(P5064_LOCINT) = LOCINT_RTC;

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < 4; i++) {
		led_display('h', 'z', '0' + i, ' ');

		/* Enable the interrupt. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_PIE | MC_REGB_SQWE |
		    MC_REGB_BINARY | MC_REGB_24HR);

		/* Wait for it to happen. */
		startctr = mips3_cp0_count_read();
		do {
			irr = REGVAL(P5064_LOCINT);
			endctr = mips3_cp0_count_read();
		} while ((irr & LOCINT_RTC) == 0);

		/* ACK. */
		bus_space_write_1(st, sh, 0, MC_REGC);
		(void) bus_space_read_1(st, sh, 1);

		/* Disable. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
		    MC_REGB_24HR);

		ctrdiff[i] = endctr - startctr;
	}

	REGVAL(P5064_LOCINT) = 0;

	/* Compute the number of cycles per second. */
	cps = ((ctrdiff[2] + ctrdiff[3]) / 2) * 16;

	/* Compute the number of ticks for hz. */
	cycles_per_hz = cps / hz;

	/* Compute the delay divisor. */
	delay_divisor = (cps / 1000000) / 2;

	printf("Timer calibration: %lu cycles/sec [(%lu, %lu) * 16]\n",
	    cps, ctrdiff[2], ctrdiff[3]);
	printf("CPU clock speed = %lu.%02luMHz "
	    "(hz cycles = %lu, delay divisor = %u)\n",
	    cps / 1000000, (cps % 1000000) / 10000,
	    cycles_per_hz, delay_divisor);
}

void *
algor_p5064_intr_establish(int irq, int (*func)(void *), void *arg)
{
	const struct p5064_irqmap *irqmap;
	struct algor_intrhand *ih;
	int s;

	irqmap = &p5064_irqmap[irq];

	KASSERT(irq == irqmap->irqidx);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = 0;
	ih->ih_irqmap = irqmap;
 
	s = splhigh();

	/*
	 * First, link it into the tables.
	 */
	LIST_INSERT_HEAD(&p5064_cpuintrs[irqmap->cpuintr].cintr_list,
	    ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (p5064_intrtab[irqmap->irqidx].intr_refcnt++ == 0) {
		p5064_irqregs[irqmap->irqreg].val |= irqmap->irqbit;
		REGVAL(p5064_irqregs[irqmap->irqreg].addr) =
		    p5064_irqregs[irqmap->irqreg].val;
	}

	splx(s);

	return (ih);
}

void
algor_p5064_intr_disestablish(void *cookie)
{
	const struct p5064_irqmap *irqmap;
	struct algor_intrhand *ih = cookie;
	int s;

	irqmap = ih->ih_irqmap;

	s = splhigh();

	/*
	 * First, remove it from the table.
	 */
	LIST_REMOVE(ih, ih_q);

	/*
	 * Now, disable it, if there is nothing remaining on the
	 * list.
	 */
	if (p5064_intrtab[irqmap->irqidx].intr_refcnt-- == 1) {
		p5064_irqregs[irqmap->irqreg].val &= ~irqmap->irqbit;
		REGVAL(p5064_irqregs[irqmap->irqreg].addr) =
		    p5064_irqregs[irqmap->irqreg].val;
	}

	splx(s);

	free(ih, M_DEVBUF);
}

void
algor_p5064_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	const struct p5064_irqmap *irqmap;
	struct algor_intrhand *ih;
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

	for (level = (NINTRS - 1); level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;
		p5064_cpuintrs[level].cintr_count.ev_count++;
		for (ih = LIST_FIRST(&p5064_cpuintrs[level].cintr_list);
		     ih != NULL; ih = LIST_NEXT(ih, ih_q)) {
			irqmap = ih->ih_irqmap;
			if (irr[irqmap->irqreg] & irqmap->irqbit) {
				p5064_intrtab[
				    irqmap->irqidx].intr_count.ev_count++;
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
		{ P5064_IRQ_ETHERNET, -1, -1, -1 },	/* 0: Ethernet */
		{ P5064_IRQ_SCSI, -1, -1, -1 },		/* 1: SCSI */
		{ -1, -1, -1, P5064_IRQ_USB },		/* 2: PCI-ISA bridge */
		{ 0, 1, 2, 3 },				/* 3: PCI slot 3 */
		{ 3, 0, 1, 2 },				/* 4: PCI slot 2 */
		{ 2, 3, 0, 1 },				/* 5: PCI slot 1 */
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

	if (ih >= NPCIIRQS)
		panic("algor_p5064_intr_string: bogus IRQ %ld\n", ih);

	return (p5064_intrnames[ih]);
}

const struct evcnt *
algor_p5064_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return (&p5064_intrtab[ih].intr_count);
}

void *
algor_p5064_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih >= NPCIIRQS)
		panic("algor_p5064_intr_establish: bogus IRQ %ld\n", ih);

	return (algor_p5064_intr_establish(ih, func, arg));
}

void
algor_p5064_pci_intr_disestablish(void *v, void *cookie)
{

	return (algor_p5064_intr_disestablish(cookie));
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
	int bus;

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	cookie = algor_p5064_intr_establish(P5064_IRQ_IDE0 + chan, func, arg);
	if (cookie == NULL)
		return (NULL);
	printf("%s: %s channel interrupting at on-board %s IRQ\n",
	    dev->dv_xname, PCIIDE_CHANNEL_NAME(chan),
	    p5064_intrnames[P5064_IRQ_IDE0 + chan]);
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
	struct algor_intrhand *ih;
	int irqidx;

	if (iirq > 15 || type == IST_NONE)
		panic("algor_p5064_isa_intr_establish: bad irq or type");

	if ((irqidx = p5064_isa_to_irqmap[iirq]) == -1)
		return (NULL);

	ih = algor_p5064_intr_establish(irqidx, func, arg);
	if (ih != NULL) {
		/* Translate it to an ISA IRQ. */
		ih->ih_irq = iirq;
	}
	return (ih);
}

void
algor_p5064_isa_intr_disestablish(void *v, void *cookie)
{
	struct algor_intrhand *ih = cookie;

	/* Translate the IRQ back to our domain. */
	ih->ih_irq = p5064_isa_to_irqmap[ih->ih_irq];

	algor_p5064_intr_disestablish(ih);
}

int
algor_p5064_isa_intr_alloc(void *v, int mask, int type, int *iirq)
{

	/* XXX */
	return (1);
}
