/*	$NetBSD: gdium_intr.c,v 1.7.6.1 2016/10/05 20:55:26 skrll Exp $	*/

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
 * Platform-specific interrupt support for the Algorithmics P-6032.
 *
 * The Algorithmics P-6032's interrupts are wired to GPIO pins
 * on the BONITO system controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gdium_intr.c,v 1.7.6.1 2016/10/05 20:55:26 skrll Exp $");

#define __INTR_PRIVATE


#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/bonito/bonitoreg.h>
#include <evbmips/gdium/gdiumvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

/*
 * The GDIUM interrupts are wired up in the following way:
 *
 *	GPIN0		ISA_NMI		(in)
 *	GPIN1		ISA_INTR	(in)
 *	GPIN2		ETH_INT~	(in)
 *	GPIN3		BONIDE_INT	(in)
 *
 *	PCI_INTA	
 *	GPIN4		ISA IRQ3	(in, also on piix4)
 *	GPIN5		ISA IRQ4	(in, also on piix4)
 *
 *	GPIO0		PIRQ A~		(in)
 *	GPIO1		PIRQ B~		(in)
 *	GPIO2		PIRQ C~		(in)
 *	GPIO3		PIRQ D~		(in)
 */

struct gdium_irqmap {
	const char *name;
	uint8_t	irqidx;
	uint8_t	flags;
};

#define	IRQ_F_INVERT	0x80	/* invert polarity */
#define	IRQ_F_EDGE	0x40	/* edge trigger */
#define	IRQ_F_INT0	0x00	/* INT0 */
#define	IRQ_F_INT1	0x01	/* INT1 */
#define	IRQ_F_INT2	0x02	/* INT2 */
#define	IRQ_F_INT3	0x03	/* INT3 */
#define	IRQ_F_INTMASK	0x07	/* INT mask */

const struct gdium_irqmap gdium_irqmap[] = {
	{ "gpio0",	GDIUM_IRQ_GPIO0,	IRQ_F_INT0 },
	{ "gpio1",	GDIUM_IRQ_GPIO1,	IRQ_F_INT0 },
	{ "gpio2",	GDIUM_IRQ_GPIO2,	IRQ_F_INT0 },
	{ "gpio3",	GDIUM_IRQ_GPIO3,	IRQ_F_INT0 },

	{ "pci inta",	GDIUM_IRQ_PCI_INTA,	IRQ_F_INT0 },
	{ "pci intb",	GDIUM_IRQ_PCI_INTB,	IRQ_F_INT0 },
	{ "pci intc",	GDIUM_IRQ_PCI_INTC,	IRQ_F_INT0 },
	{ "pci intd",	GDIUM_IRQ_PCI_INTD,	IRQ_F_INT0 },
   
	{ "pci perr",	GDIUM_IRQ_PCI_PERR,	IRQ_F_EDGE|IRQ_F_INT1 },
	{ "pci serr",	GDIUM_IRQ_PCI_SERR,	IRQ_F_EDGE|IRQ_F_INT1 },

	{ "denali",	GDIUM_IRQ_DENALI,	IRQ_F_INT1 },

	{ "mips int0",	GDIUM_IRQ_INT0,		IRQ_F_INT0 },
	{ "mips int1",	GDIUM_IRQ_INT1,		IRQ_F_INT1 },
	{ "mips int2",	GDIUM_IRQ_INT2,		IRQ_F_INT2 },
	{ "mips int3",	GDIUM_IRQ_INT3,		IRQ_F_INT3 },
};

struct gdium_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct gdium_intrhead gdium_intrtab[__arraycount(gdium_irqmap)];

#define	NINTRS			2	/* MIPS INT0 - INT1 */

struct gdium_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
	int cintr_refcnt;
};

struct gdium_cpuintr gdium_cpuintrs[NINTRS];
const char * const gdium_cpuintrnames[NINTRS] = {
	"int 0 (pci)",
	"int 1 (errors)",
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map gdium_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4,
	[IPL_SCHED] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =            MIPS_INT_MASK,
    },
};

int	gdium_pci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
const char *gdium_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *gdium_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*gdium_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	gdium_pci_intr_disestablish(void *, void *);
void	gdium_pci_conf_interrupt(void *, int, int, int, int, int *);

void
evbmips_intr_init(void)
{
	struct gdium_config * const gc = &gdium_configuration;
	struct bonito_config *bc = &gc->gc_bonito;
	const struct gdium_irqmap *irqmap;
	uint32_t intbit;
	size_t i;

	ipl_sr_map = gdium_ipl_sr_map;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&gdium_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&gdium_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", gdium_cpuintrnames[i]);
	}
	//evcnt_attach_static(&mips_int5_evcnt);

	for (i = 0; i < __arraycount(gdium_irqmap); i++) {
		irqmap = &gdium_irqmap[i];
		intbit = 1 << irqmap->irqidx;

		evcnt_attach_dynamic(&gdium_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "bonito", irqmap->name);

		if (irqmap->irqidx < 4)
			bc->bc_gpioIE |= intbit;
		if (irqmap->flags & IRQ_F_INVERT)
			bc->bc_intPol |= intbit;
		if (irqmap->flags & IRQ_F_EDGE)
			bc->bc_intEdge |= intbit;
		if ((irqmap->flags & IRQ_F_INTMASK) == IRQ_F_INT1)
			bc->bc_intSteer |= intbit;

		REGVAL(BONITO_INTENCLR) = intbit;
	}

	REGVAL(BONITO_GPIOIE) = bc->bc_gpioIE;
	REGVAL(BONITO_INTEDGE) = bc->bc_intEdge;
	REGVAL(BONITO_INTSTEER) = bc->bc_intSteer;
	REGVAL(BONITO_INTPOL) = bc->bc_intPol;

	gc->gc_pc.pc_intr_v = NULL;
	gc->gc_pc.pc_intr_map = gdium_pci_intr_map;
	gc->gc_pc.pc_intr_string = gdium_pci_intr_string;
	gc->gc_pc.pc_intr_evcnt = gdium_pci_intr_evcnt;
	gc->gc_pc.pc_intr_establish = gdium_pci_intr_establish;
	gc->gc_pc.pc_intr_disestablish = gdium_pci_intr_disestablish;
	gc->gc_pc.pc_conf_interrupt = gdium_pci_conf_interrupt;

	/* We let the PCI-ISA bridge code handle this. */
	gc->gc_pc.pc_pciide_compat_intr_establish = NULL;
}

void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	const struct gdium_irqmap *irqmap;
	struct evbmips_intrhand *ih;
	int level;
	int s;

	irqmap = &gdium_irqmap[irq];
	KASSERT(irq < __arraycount(gdium_irqmap));

	KASSERT(irq == irqmap->irqidx);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ih == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;

	s = splhigh();

	/*
	 * First, link it into the tables.
	 */
	level = (irqmap->flags & IRQ_F_INT1) != 0;
	LIST_INSERT_HEAD(&gdium_cpuintrs[level].cintr_list, ih, ih_q);
	gdium_cpuintrs[level].cintr_refcnt++;

	/*
	 * Now enable it.
	 */
	if (gdium_intrtab[ih->ih_irq].intr_refcnt++ == 0)
		REGVAL(BONITO_INTENSET) = (1 << ih->ih_irq);

	splx(s);

	return (ih);
}

void
evbmips_intr_disestablish(void *cookie)
{
	const struct gdium_irqmap *irqmap;
	struct evbmips_intrhand *ih = cookie;
	int s;

	irqmap = &gdium_irqmap[ih->ih_irq];

	s = splhigh();

	/*
	 * First, remove it from the table.
	 */
	LIST_REMOVE(ih, ih_q);
	gdium_cpuintrs[(irqmap->flags & IRQ_F_INT1) != 0].cintr_refcnt--;

	/*
	 * Now, disable it, if there is nothing remaining on the
	 * list.
	 */
	if (gdium_intrtab[ih->ih_irq].intr_refcnt-- == 1)
		REGVAL(BONITO_INTENCLR) = (1 << ih->ih_irq);

	splx(s);

	free(ih, M_DEVBUF);
}

void
evbmips_iointr(int ipl, uint32_t ipending, struct clockframe *cf)
{
	struct evbmips_intrhand *ih;
	int level;
	uint32_t isr;

	/*
	 * Read the interrupt pending registers, mask them with the
	 * ones we have enabled, and service them in order of decreasing
	 * priority.
	 */
	isr = REGVAL(BONITO_INTISR) & REGVAL(BONITO_INTEN);
	for (level = 1; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_4 << level)) == 0)
			continue;
		gdium_cpuintrs[level].cintr_count.ev_count++;
		LIST_FOREACH (ih, &gdium_cpuintrs[level].cintr_list, ih_q) {
			if (isr & (1 << ih->ih_irq)) {
				gdium_intrtab[ih->ih_irq].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);
			}
		}
	}
}

/*****************************************************************************
 * PCI interrupt support
 *****************************************************************************/

int
gdium_pci_intr_map(const struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	static const int8_t pciirqmap[5/*device*/] = {
	    GDIUM_IRQ_PCI_INTC, /* 13: PCI 802.11 */
	    GDIUM_IRQ_PCI_INTA, /* 14: SM501 */
	    GDIUM_IRQ_PCI_INTB, /* 15: NEC USB (2 func) */
	    GDIUM_IRQ_PCI_INTD, /* 16: Ethernet */
	    GDIUM_IRQ_PCI_INTC, /* 17: NEC USB (2 func) */
	};
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("gdium_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (device < 13 || device > 17) {
		printf("gdium_pci_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	*ihp = pciirqmap[device - 13];
	return (0);
}

const char *
gdium_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{

	if (ih >= __arraycount(gdium_irqmap))
		panic("gdium_intr_string: bogus IRQ %ld", ih);

	strlcpy(buf, gdium_irqmap[ih].name, len);
	return buf;
}

const struct evcnt *
gdium_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return &gdium_intrtab[ih].intr_count;
}

void *
gdium_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih >= __arraycount(gdium_irqmap))
		panic("gdium_pci_intr_establish: bogus IRQ %ld", ih);

	return evbmips_intr_establish(ih, func, arg);
}

void
gdium_pci_intr_disestablish(void *v, void *cookie)
{

	return (evbmips_intr_disestablish(cookie));
}

void
gdium_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}
