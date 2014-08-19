/*	$NetBSD: algor_p4032_intr.c,v 1.24.12.1 2014/08/20 00:02:41 tls Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: algor_p4032_intr.c,v 1.24.12.1 2014/08/20 00:02:41 tls Exp $");

#include "opt_ddb.h"
#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <algor/autoconf.h>

#include <mips/locore.h>

#include <dev/ic/mc146818reg.h>

#include <algor/algor/algor_p4032reg.h>
#include <algor/algor/algor_p4032var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define	REGVAL(x)	*((volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

struct p4032_irqreg {
	bus_addr_t	addr;
	u_int32_t	val;
};

#define	IRQREG_8BIT		0
#define	IRQREG_ERROR		1
#define	IRQREG_PCI		2
#define	NIRQREG			3

struct p4032_irqreg p4032_irqregs[NIRQREG] = {
	{ P4032_IRR0,		0 },
	{ P4032_IRR1,		0 },
	{ P4032_IRR2,		0 },
};

#define	NSTEERREG		3

struct p4032_irqreg p4032_irqsteer[NSTEERREG] = {
	{ P4032_XBAR0,		0 },
	{ P4032_XBAR1,		0 },
	{ P4032_XBAR2,		0 },
};

#define	NPCIIRQS		4

/* See algor_p4032var.h */
#define	N8BITIRQS		8

#define	IRQMAP_PCIBASE		0
#define	IRQMAP_8BITBASE		NPCIIRQS
#define	NIRQMAPS		(IRQMAP_8BITBASE + N8BITIRQS)

const char * const p4032_intrnames[NIRQMAPS] = {
	/*
	 * PCI INTERRUPTS
	 */
	"PCIIRQ 0",
	"PCIIRQ 1",
	"PCIIRQ 2",
	"PCIIRQ 3",

	/*
	 * 8-BIT DEVICE INTERRUPTS
	 */
	"PCI ctlr",
	"floppy",
	"pckbc",
	"com 1",
	"com 2",
	"centronics",
	"gpio",
	"mcclock",
};

struct p4032_irqmap {
	int	irqidx;         
	int	cpuintr;
	int	irqreg;
	int	irqbit;
	int	xbarreg;        
	int	xbarshift;
};      

const struct p4032_irqmap p4032_irqmap[NIRQMAPS] = {
	/*
	 * PCI INTERRUPTS
	 */
	/* PCIIRQ 0 */
	{ 0,			0,
	  IRQREG_PCI,		IRR2_PCIIRQ0,
	  2,			0 },

	/* PCIIRQ 1 */
	{ 1,			0,
	  IRQREG_PCI,		IRR2_PCIIRQ1,
	  2,			2 },

	/* PCIIRQ 2 */
	{ 2,			0,
	  IRQREG_PCI,		IRR2_PCIIRQ2,
	  2,			4 },

	/* PCIIRQ 3 */
	{ 3,			0,
	  IRQREG_PCI,		IRR2_PCIIRQ3,
	  2,			6 },

	/*
	 * 8-BIT DEVICE INTERRUPTS
	 */
	{ P4032_IRQ_PCICTLR,	1,
	  IRQREG_8BIT,		IRR0_PCICTLR,
	  0,			0 },

	{ P4032_IRQ_FLOPPY,	1,
	  IRQREG_8BIT,		IRR0_FLOPPY,
	  0,			2 },

	{ P4032_IRQ_PCKBC,	1,
	  IRQREG_8BIT,		IRR0_PCKBC,
	  0,			4 },

	{ P4032_IRQ_COM1,	1,
	  IRQREG_8BIT,		IRR0_COM1,
	  0,			6 },

	{ P4032_IRQ_COM2,	1,
	  IRQREG_8BIT,		IRR0_COM2,
	  1,			0 },

	{ P4032_IRQ_LPT,	1,
	  IRQREG_8BIT,		IRR0_LPT,
	  1,			2 },

	{ P4032_IRQ_GPIO,	1,
	  IRQREG_8BIT,		IRR0_GPIO,
	  1,			4 },

	{ P4032_IRQ_RTC,	1,
	  IRQREG_8BIT,		IRR0_RTC,
	  1,			6 },
};

struct p4032_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct p4032_intrhead p4032_intrtab[NIRQMAPS];

#define	NINTRS			2	/* MIPS INT0 - INT1 */


struct p4032_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
};

struct p4032_cpuintr p4032_cpuintrs[NINTRS];
const char * const p4032_cpuintrnames[NINTRS] = {
	"int 0 (pci)",
	"int 1 (8-bit)",
};

const char * const p4032_intrgroups[NINTRS] = {
	"pci",
	"8-bit",
};

void	*algor_p4032_intr_establish(int, int (*)(void *), void *);
void	algor_p4032_intr_disestablish(void *);

int	algor_p4032_pci_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *algor_p4032_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *algor_p4032_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*algor_p4032_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	algor_p4032_pci_intr_disestablish(void *, void *);
void	algor_p4032_pci_conf_interrupt(void *, int, int, int, int, int *);

void	algor_p4032_iointr(int, vaddr_t, uint32_t);

void
algor_p4032_intr_init(struct p4032_config *acp)
{
	const struct p4032_irqmap *irqmap;
	int i;

	for (i = 0; i < NIRQREG; i++)
		REGVAL(p4032_irqregs[i].addr) = p4032_irqregs[i].val;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&p4032_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&p4032_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", p4032_cpuintrnames[i]);
	}

	for (i = 0; i < NIRQMAPS; i++) {
		irqmap = &p4032_irqmap[i];

		p4032_irqsteer[irqmap->xbarreg].val |=
		    irqmap->cpuintr << irqmap->xbarshift;

		evcnt_attach_dynamic(&p4032_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, p4032_intrgroups[irqmap->cpuintr],
		    p4032_intrnames[i]);
	}

	for (i = 0; i < NSTEERREG; i++)
		REGVAL(p4032_irqsteer[i].addr) = p4032_irqsteer[i].val;

	acp->ac_pc.pc_intr_v = NULL;
	acp->ac_pc.pc_intr_map = algor_p4032_pci_intr_map;
	acp->ac_pc.pc_intr_string = algor_p4032_pci_intr_string;
	acp->ac_pc.pc_intr_evcnt = algor_p4032_pci_intr_evcnt;
	acp->ac_pc.pc_intr_establish = algor_p4032_pci_intr_establish;
	acp->ac_pc.pc_intr_disestablish = algor_p4032_pci_intr_disestablish;
	acp->ac_pc.pc_conf_interrupt = algor_p4032_pci_conf_interrupt;
	acp->ac_pc.pc_pciide_compat_intr_establish = NULL;

	algor_intr_establish = algor_p4032_intr_establish;
	algor_intr_disestablish = algor_p4032_intr_disestablish;
	algor_iointr = algor_p4032_iointr;
}

void
algor_p4032_cal_timer(bus_space_tag_t st, bus_space_handle_t sh)
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

	REGVAL(P4032_IRR0) = IRR0_RTC;

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
			irr = REGVAL(P4032_IRR0);
			endctr = mips3_cp0_count_read();
		} while ((irr & IRR0_RTC) == 0);

		/* ACK. */
		bus_space_write_1(st, sh, 0, MC_REGC);
		(void) bus_space_read_1(st, sh, 1);

		/* Disable. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
		    MC_REGB_24HR);

		ctrdiff[i] = endctr - startctr;
	}

	REGVAL(P4032_IRR0) = 0;

	/* Update CPU frequency values */
	cps = ((ctrdiff[2] + ctrdiff[3]) / 2) * 16;
	/* XXX mips_cpu_flags isn't set here; assume CPU_MIPS_DOUBLE_COUNT */
	curcpu()->ci_cpu_freq = cps * 2;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + (1000000 / 2)) / 1000000);
	/* XXX assume CPU_MIPS_DOUBLE_COUNT */
	curcpu()->ci_cycles_per_hz /= 2;
	curcpu()->ci_divisor_delay /= 2;

	printf("Timer calibration: %lu cycles/sec [(%lu, %lu) * 16]\n",
	    cps, ctrdiff[2], ctrdiff[3]);
	printf("CPU clock speed = %lu.%02luMHz "
	    "(hz cycles = %lu, delay divisor = %lu)\n",
	    curcpu()->ci_cpu_freq / 1000000,
	    (curcpu()->ci_cpu_freq % 1000000) / 10000,
	    curcpu()->ci_cycles_per_hz, curcpu()->ci_divisor_delay);
}

void *
algor_p4032_intr_establish(int irq, int (*func)(void *), void *arg)
{
	const struct p4032_irqmap *irqmap;
	struct evbmips_intrhand *ih;
	int s;

	irqmap = &p4032_irqmap[irq];

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
	LIST_INSERT_HEAD(&p4032_cpuintrs[irqmap->cpuintr].cintr_list,
	    ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (p4032_intrtab[irqmap->irqidx].intr_refcnt++ == 0) {
		p4032_irqregs[irqmap->irqreg].val |= irqmap->irqbit;
		REGVAL(p4032_irqregs[irqmap->irqreg].addr) =
		    p4032_irqregs[irqmap->irqreg].val;
	}

	splx(s);

	return (ih);
}

void
algor_p4032_intr_disestablish(void *cookie)
{
	const struct p4032_irqmap *irqmap;
	struct evbmips_intrhand *ih = cookie;
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
	if (p4032_intrtab[irqmap->irqidx].intr_refcnt-- == 1) {
		p4032_irqregs[irqmap->irqreg].val &= ~irqmap->irqbit;
		REGVAL(p4032_irqregs[irqmap->irqreg].addr) =
		    p4032_irqregs[irqmap->irqreg].val;
	}

	splx(s);

	free(ih, M_DEVBUF);
}

void
algor_p4032_iointr(int ipl, vaddr_t pc, u_int32_t ipending)
{
	const struct p4032_irqmap *irqmap;
	struct evbmips_intrhand *ih;
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

	/* Do floppy DMA request interrupts. */
	if (ipending & MIPS_INT_MASK_3) {
		/*
		 * XXX Hi, um, yah, we need to deal with
		 * XXX the floppy interrupt here.
		 */

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

	for (level = (NINTRS - 1); level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;
		p4032_cpuintrs[level].cintr_count.ev_count++;
		for (ih = LIST_FIRST(&p4032_cpuintrs[level].cintr_list);
		     ih != NULL; ih = LIST_NEXT(ih, ih_q)) {
			irqmap = ih->ih_irqmap;
			if (irr[irqmap->irqreg] & irqmap->irqbit) {
				p4032_intrtab[
				    irqmap->irqidx].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);
			}
		}
	}
}

/*****************************************************************************
 * PCI interrupt support
 *****************************************************************************/

int
algor_p4032_pci_intr_map(const struct pci_attach_args *pa,
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
algor_p4032_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{

	if (ih >= NPCIIRQS)
		panic("algor_p4032_intr_string: bogus IRQ %ld", ih);

	strlcpy(buf, p4032_intrnames[ih], len);
	return buf;
}

const struct evcnt *
algor_p4032_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return (&p4032_intrtab[ih].intr_count);
}

void *
algor_p4032_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih >= NPCIIRQS)
		panic("algor_p4032_intr_establish: bogus IRQ %ld", ih);

	return (algor_p4032_intr_establish(ih, func, arg));
}

void
algor_p4032_pci_intr_disestablish(void *v, void *cookie)
{

	return (algor_p4032_intr_disestablish(cookie));
}

void
algor_p4032_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}
