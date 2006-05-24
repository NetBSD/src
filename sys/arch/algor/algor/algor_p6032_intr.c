/*	$NetBSD: algor_p6032_intr.c,v 1.7.8.1 2006/05/24 10:56:32 yamt Exp $	*/

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
 * Platform-specific interrupt support for the Algorithmics P-6032.
 *
 * The Algorithmics P-6032's interrupts are wired to GPIO pins
 * on the BONITO system controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: algor_p6032_intr.c,v 1.7.8.1 2006/05/24 10:56:32 yamt Exp $");

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

#include <algor/algor/algor_p6032reg.h>
#include <algor/algor/algor_p6032var.h>

#include <algor/algor/clockvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/isa/isavar.h>

/*
 * The P-6032 interrupts are wired up in the following way:
 *
 *	GPIN0		ISA_NMI		(in)
 *	GPIN1		ISA_INTR	(in)
 *	GPIN2		ETH_INT~	(in)
 *	GPIN3		BONIDE_INT	(in)
 *
 *	GPIN4		ISA IRQ3	(in, also on piix4)
 *	GPIN5		ISA IRQ4	(in, also on piix4)
 *
 *	GPIO0		PIRQ A~		(in)
 *	GPIO1		PIRQ B~		(in)
 *	GPIO2		PIRQ C~		(in)
 *	GPIO3		PIRQ D~		(in)
 */

#define	NIRQMAPS	10

const char *p6032_intrnames[NIRQMAPS] = {
	"gpin 0",
	"gpin 1",
	"gpin 2",
	"gpin 3",

	"gpin 4",
	"gpin 5",

	"gpio 0",
	"gpio 1",
	"gpio 2",
	"gpio 3",
};

struct p6032_irqmap {
	int	irqidx;
	uint32_t intbit;
	uint32_t gpioiebit;
	int	flags;
};

#define	IRQ_F_INVERT	0x01	/* invert polarity */
#define	IRQ_F_EDGE	0x02	/* edge trigger */
#define	IRQ_F_INT1	0x04	/* INT1, else INT0 */

const struct p6032_irqmap p6032_irqmap[NIRQMAPS] = {
	/* ISA NMI */
	{ P6032_IRQ_GPIN0,	BONITO_ICU_GPIN(0),
	  BONITO_GPIO_INR(0),	IRQ_F_INT1 },

	/* ISA bridge */
	{ P6032_IRQ_GPIN1,	BONITO_ICU_GPIN(1),
	  BONITO_GPIO_INR(1),	IRQ_F_INT1 },

	/* Ethernet */
	{ P6032_IRQ_GPIN2,	BONITO_ICU_GPIN(2),
	  BONITO_GPIO_INR(2),	IRQ_F_INVERT },

	/* BONITO IDE */
	{ P6032_IRQ_GPIN3,	BONITO_ICU_GPIN(3),
	  BONITO_GPIO_INR(3),	0 },

	/* ISA IRQ3 */
	{ P6032_IRQ_GPIN4,	BONITO_ICU_GPIN(4),
	  BONITO_GPIO_INR(4),	IRQ_F_INT1 },

	/* ISA IRQ4 */
	{ P6032_IRQ_GPIN5,	BONITO_ICU_GPIN(5),
	  BONITO_GPIO_INR(5),	IRQ_F_INT1 },

	/* PIRQ A */
	{ P6032_IRQ_GPIO0,	BONITO_ICU_GPIO(0),
	  BONITO_GPIO_IOW(0),	IRQ_F_INVERT },

	/* PIRQ B */
	{ P6032_IRQ_GPIO1,	BONITO_ICU_GPIO(1),
	  BONITO_GPIO_IOW(1),	IRQ_F_INVERT },

	/* PIRQ C */
	{ P6032_IRQ_GPIO2,	BONITO_ICU_GPIO(2),
	  BONITO_GPIO_IOW(2),	IRQ_F_INVERT },

	/* PIRQ D */
	{ P6032_IRQ_GPIO3,	BONITO_ICU_GPIO(3),
	  BONITO_GPIO_IOW(3),	IRQ_F_INVERT },
};

struct p6032_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct p6032_intrhead p6032_intrtab[NIRQMAPS];

#define	NINTRS			2	/* MIPS INT0 - INT1 */

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given software interrupt priority level.
 * Hardware ipls are port/board specific.
 */
const uint32_t mips_ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTSERIAL */
};

struct p6032_cpuintr {
	LIST_HEAD(, algor_intrhand) cintr_list;
	struct evcnt cintr_count;
};

struct p6032_cpuintr p6032_cpuintrs[NINTRS];
const char *p6032_cpuintrnames[NINTRS] = {
	"int 0 (pci)",
	"int 1 (isa)",
};

void	*algor_p6032_intr_establish(int, int (*)(void *), void *);
void	algor_p6032_intr_disestablish(void *);

int	algor_p6032_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *algor_p6032_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *algor_p6032_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*algor_p6032_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	algor_p6032_pci_intr_disestablish(void *, void *);
void	algor_p6032_pci_conf_interrupt(void *, int, int, int, int, int *);

void	algor_p6032_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void
algor_p6032_intr_init(struct p6032_config *acp)
{
	struct bonito_config *bc = &acp->ac_bonito;
	const struct p6032_irqmap *irqmap;
	int i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&p6032_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&p6032_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", p6032_cpuintrnames[i]);
	}
	evcnt_attach_static(&mips_int5_evcnt);

	for (i = 0; i <= NIRQMAPS; i++) {
		irqmap = &p6032_irqmap[i];

		evcnt_attach_dynamic(&p6032_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "bonito", p6032_intrnames[i]);

		bc->bc_gpioIE |= irqmap->gpioiebit;
		if (irqmap->flags & IRQ_F_INVERT)
			bc->bc_intPol |= irqmap->intbit;
		if (irqmap->flags & IRQ_F_EDGE)
			bc->bc_intEdge |= irqmap->intbit;
		if (irqmap->flags & IRQ_F_INT1)
			bc->bc_intSteer |= irqmap->intbit;

		REGVAL(BONITO_INTENCLR) = irqmap->intbit;
	}

	REGVAL(BONITO_GPIOIE) = bc->bc_gpioIE;
	REGVAL(BONITO_INTEDGE) = bc->bc_intEdge;
	REGVAL(BONITO_INTSTEER) = bc->bc_intSteer;
	REGVAL(BONITO_INTPOL) = bc->bc_intPol;

	acp->ac_pc.pc_intr_v = NULL;
	acp->ac_pc.pc_intr_map = algor_p6032_pci_intr_map;
	acp->ac_pc.pc_intr_string = algor_p6032_pci_intr_string;
	acp->ac_pc.pc_intr_evcnt = algor_p6032_pci_intr_evcnt;
	acp->ac_pc.pc_intr_establish = algor_p6032_pci_intr_establish;
	acp->ac_pc.pc_intr_disestablish = algor_p6032_pci_intr_disestablish;
	acp->ac_pc.pc_conf_interrupt = algor_p6032_pci_conf_interrupt;

	/* We let the PCI-ISA bridge code handle this. */
	acp->ac_pc.pc_pciide_compat_intr_establish = NULL;

	algor_intr_establish = algor_p6032_intr_establish;
	algor_intr_disestablish = algor_p6032_intr_disestablish;
	algor_iointr = algor_p6032_iointr;
}

void
algor_p6032_cal_timer(bus_space_tag_t st, bus_space_handle_t sh)
{
	u_long ctrdiff[4], startctr, endctr, cps;
	u_int8_t regc;
	int i;

	/* Disable interrupts first. */
	bus_space_write_1(st, sh, 0, MC_REGB);
	bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	/* Initialize for 16Hz. */
	bus_space_write_1(st, sh, 0, MC_REGA);
	bus_space_write_1(st, sh, 1, MC_BASE_32_KHz | MC_RATE_16_Hz);

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < 4; i++) {
		led_display('h', 'z', '0' + i, ' ');

		/* Enable the interrupt. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_PIE | MC_REGB_SQWE |
		    MC_REGB_BINARY | MC_REGB_24HR);

		/* Go to REGC. */
		bus_space_write_1(st, sh, 0, MC_REGC);

		/* Wait for it to happen. */
		startctr = mips3_cp0_count_read();
		do {
			regc = bus_space_read_1(st, sh, 1);
			endctr = mips3_cp0_count_read();
		} while ((regc & MC_REGC_IRQF) == 0);

		/* Already ACK'd. */

		/* Disable. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
		    MC_REGB_24HR);

		ctrdiff[i] = endctr - startctr;
	}

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
algor_p6032_intr_establish(int irq, int (*func)(void *), void *arg)
{
	const struct p6032_irqmap *irqmap;
	struct algor_intrhand *ih;
	int s;

	irqmap = &p6032_irqmap[irq];

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
	if (irqmap->flags & IRQ_F_INT1)
		LIST_INSERT_HEAD(&p6032_cpuintrs[1].cintr_list, ih, ih_q);
	else
		LIST_INSERT_HEAD(&p6032_cpuintrs[0].cintr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (p6032_intrtab[irqmap->irqidx].intr_refcnt++ == 0)
		REGVAL(BONITO_INTENSET) = irqmap->intbit;

	splx(s);

	return (ih);
}

void
algor_p6032_intr_disestablish(void *cookie)
{
	const struct p6032_irqmap *irqmap;
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
	if (p6032_intrtab[irqmap->irqidx].intr_refcnt-- == 1)
		REGVAL(BONITO_INTENCLR) = irqmap->intbit;

	splx(s);

	free(ih, M_DEVBUF);
}

void
algor_p6032_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	const struct p6032_irqmap *irqmap;
	struct algor_intrhand *ih;
	int level;
	u_int32_t isr;

	/* Check for DEBUG interrupts. */
	if (ipending & MIPS_INT_MASK_3) {
#ifdef DDB
		printf("Debug switch -- entering debugger\n");
		led_display('D','D','B',' ');
		Debugger();
		led_display('N','B','S','D');
#else
		printf("Debug switch ignored -- "
		    "no debugger configured\n");
#endif

		cause &= ~MIPS_INT_MASK_3;
	}

	/*
	 * Read the interrupt pending registers, mask them with the
	 * ones we have enabled, and service them in order of decreasing
	 * priority.
	 */
	isr = REGVAL(BONITO_INTISR) & REGVAL(BONITO_INTEN);

	for (level = 1; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;
		p6032_cpuintrs[level].cintr_count.ev_count++;
		for (ih = LIST_FIRST(&p6032_cpuintrs[level].cintr_list);
		     ih != NULL; ih = LIST_NEXT(ih, ih_q)) {
			irqmap = ih->ih_irqmap;
			if (isr & irqmap->intbit) {
				p6032_intrtab[
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
algor_p6032_pci_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	static const int pciirqmap[6/*device*/][4/*pin*/] = {
	    { P6032_IRQ_GPIO0, P6032_IRQ_GPIO1,
	      P6032_IRQ_GPIO2, P6032_IRQ_GPIO3 }, /* 13: slot 2 (p9) */

	    { P6032_IRQ_GPIO1, P6032_IRQ_GPIO2,
	      P6032_IRQ_GPIO3, P6032_IRQ_GPIO0 }, /* 14: slot 3 (p10) */

	    { P6032_IRQ_GPIO2, P6032_IRQ_GPIO3,
	      P6032_IRQ_GPIO0, P6032_IRQ_GPIO1 }, /* 15: slot 4 (p11) */

	    { P6032_IRQ_GPIN2, -1,
	      -1,              -1 },              /* 16: Ethernet */

	    { P6032_IRQ_GPIO0, P6032_IRQ_GPIO1,
	      P6032_IRQ_GPIO2, P6032_IRQ_GPIO3 }, /* 17: southbridge */

	    { P6032_IRQ_GPIO3, P6032_IRQ_GPIO0,
	      P6032_IRQ_GPIO1, P6032_IRQ_GPIO2 }, /* 18: slot 1 (p8) */
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
		printf("algor_p6032_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (device < 13 || device > 18) {
		printf("algor_p6032_pci_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	irq = pciirqmap[device - 13][buspin - 1];
	if (irq == -1) {
		printf("algor_p6032_pci_intr_map: no mapping for "
		    "device %d pin %d\n", device, buspin);
		return (1);
	}

	*ihp = irq;
	return (0);
}

const char *
algor_p6032_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	if (ih >= NIRQMAPS)
		panic("algor_p6032_intr_string: bogus IRQ %ld", ih);

	return (p6032_intrnames[ih]);
}

const struct evcnt *
algor_p6032_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return (&p6032_intrtab[ih].intr_count);
}

void *
algor_p6032_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih >= NIRQMAPS)
		panic("algor_p6032_intr_establish: bogus IRQ %ld", ih);

	return (algor_p6032_intr_establish(ih, func, arg));
}

void
algor_p6032_pci_intr_disestablish(void *v, void *cookie)
{

	return (algor_p6032_intr_disestablish(cookie));
}

void
algor_p6032_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}
