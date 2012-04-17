/* $NetBSD: sio_pic.c,v 1.41.2.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

__KERNEL_RCSID(0, "$NetBSD: sio_pic.c,v 1.41.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/syslog.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/cy82c693reg.h>
#include <dev/pci/cy82c693var.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <alpha/pci/siovar.h>

#include "sio.h"

/*
 * To add to the long history of wonderful PROM console traits,
 * AlphaStation PROMs don't reset themselves completely on boot!
 * Therefore, if an interrupt was turned on when the kernel was
 * started, we're not going to EVER turn it off...  I don't know
 * what will happen if new interrupts (that the PROM console doesn't
 * want) are turned on.  I'll burn that bridge when I come to it.
 */
#define	BROKEN_PROM_CONSOLE

/*
 * Private functions and variables.
 */

bus_space_tag_t sio_iot;
pci_chipset_tag_t sio_pc;
bus_space_handle_t sio_ioh_icu1, sio_ioh_icu2;

#define	ICU_LEN		16		/* number of ISA IRQs */

static struct alpha_shared_intr *sio_intr;

#ifndef STRAY_MAX
#define	STRAY_MAX	5
#endif

#ifdef BROKEN_PROM_CONSOLE
/*
 * If prom console is broken, must remember the initial interrupt
 * settings and enforce them.  WHEE!
 */
uint8_t initial_ocw1[2];
uint8_t initial_elcr[2];
#endif

void		sio_setirqstat(int, int, int);

uint8_t	(*sio_read_elcr)(int);
void		(*sio_write_elcr)(int, uint8_t);
static void	specific_eoi(int);
#ifdef BROKEN_PROM_CONSOLE
void		sio_intr_shutdown(void *);
#endif

/******************** i82378 SIO ELCR functions ********************/

int		i82378_setup_elcr(void);
uint8_t	i82378_read_elcr(int);
void		i82378_write_elcr(int, uint8_t);

bus_space_handle_t sio_ioh_elcr;

int
i82378_setup_elcr(void)
{
	int rv;

	/*
	 * We could probe configuration space to see that there's
	 * actually an SIO present, but we are using this as a
	 * fall-back in case nothing else matches.
	 */

	rv = bus_space_map(sio_iot, 0x4d0, 2, 0, &sio_ioh_elcr);

	if (rv == 0) {
		sio_read_elcr = i82378_read_elcr;
		sio_write_elcr = i82378_write_elcr;
	}

	return (rv);
}

uint8_t
i82378_read_elcr(int elcr)
{

	return (bus_space_read_1(sio_iot, sio_ioh_elcr, elcr));
}

void
i82378_write_elcr(int elcr, uint8_t val)
{

	bus_space_write_1(sio_iot, sio_ioh_elcr, elcr, val);
}

/******************** Cypress CY82C693 ELCR functions ********************/

int		cy82c693_setup_elcr(void);
uint8_t	cy82c693_read_elcr(int);
void		cy82c693_write_elcr(int, uint8_t);

const struct cy82c693_handle *sio_cy82c693_handle;

int
cy82c693_setup_elcr(void)
{
	int device, maxndevs;
	pcitag_t tag;
	pcireg_t id;

	/*
	 * Search PCI configuration space for a Cypress CY82C693.
	 *
	 * Note we can make some assumptions about our bus number
	 * here, because:
	 *
	 *	(1) there can be at most one ISA/EISA bridge per PCI bus, and
	 *
	 *	(2) any ISA/EISA bridges must be attached to primary PCI
	 *	    busses (i.e. bus zero).
	 */

	maxndevs = pci_bus_maxdevs(sio_pc, 0);

	for (device = 0; device < maxndevs; device++) {
		tag = pci_make_tag(sio_pc, 0, device, 0);
		id = pci_conf_read(sio_pc, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0)
			continue;

		if (PCI_VENDOR(id) != PCI_VENDOR_CONTAQ ||
		    PCI_PRODUCT(id) != PCI_PRODUCT_CONTAQ_82C693)
			continue;

		/*
		 * Found one!
		 */

#if 0
		printf("cy82c693_setup_elcr: found 82C693 at device %d\n",
		    device);
#endif

		sio_cy82c693_handle = cy82c693_init(sio_iot);
		sio_read_elcr = cy82c693_read_elcr;
		sio_write_elcr = cy82c693_write_elcr;

		return (0);
	}

	/*
	 * Didn't find a CY82C693.
	 */
	return (ENODEV);
}

uint8_t
cy82c693_read_elcr(int elcr)
{

	return (cy82c693_read(sio_cy82c693_handle, CONFIG_ELCR1 + elcr));
}

void
cy82c693_write_elcr(int elcr, uint8_t val)
{

	cy82c693_write(sio_cy82c693_handle, CONFIG_ELCR1 + elcr, val);
}

/******************** ELCR access function configuration ********************/

/*
 * Put the Intel SIO at the end, so we fall back on it if we don't
 * find anything else.  If any of the non-Intel functions find a
 * matching device, but are unable to map it for whatever reason,
 * they should panic.
 */

int (*const sio_elcr_setup_funcs[])(void) = {
	cy82c693_setup_elcr,
	i82378_setup_elcr,
	NULL,
};

/******************** Shared SIO/Cypress functions ********************/

void
sio_setirqstat(int irq, int enabled, int type)
{
	uint8_t ocw1[2], elcr[2];
	int icu, bit;

#if 0
	printf("sio_setirqstat: irq %d: %s, %s\n", irq,
	    enabled ? "enabled" : "disabled", isa_intr_typename(type));
#endif

	icu = irq / 8;
	bit = irq % 8;

	ocw1[0] = bus_space_read_1(sio_iot, sio_ioh_icu1, 1);
	ocw1[1] = bus_space_read_1(sio_iot, sio_ioh_icu2, 1);
	elcr[0] = (*sio_read_elcr)(0);				/* XXX */
	elcr[1] = (*sio_read_elcr)(1);				/* XXX */

	/*
	 * interrupt enable: set bit to mask (disable) interrupt.
	 */
	if (enabled)
		ocw1[icu] &= ~(1 << bit);
	else
		ocw1[icu] |= 1 << bit;

	/*
	 * interrupt type select: set bit to get level-triggered.
	 */
	if (type == IST_LEVEL)
		elcr[icu] |= 1 << bit;
	else
		elcr[icu] &= ~(1 << bit);

#ifdef not_here
	/* see the init function... */
	ocw1[0] &= ~0x04;		/* always enable IRQ2 on first PIC */
	elcr[0] &= ~0x07;		/* IRQ[0-2] must be edge-triggered */
	elcr[1] &= ~0x21;		/* IRQ[13,8] must be edge-triggered */
#endif

	bus_space_write_1(sio_iot, sio_ioh_icu1, 1, ocw1[0]);
	bus_space_write_1(sio_iot, sio_ioh_icu2, 1, ocw1[1]);
	(*sio_write_elcr)(0, elcr[0]);				/* XXX */
	(*sio_write_elcr)(1, elcr[1]);				/* XXX */
}

void
sio_intr_setup(pci_chipset_tag_t pc, bus_space_tag_t iot)
{
	char *cp;
	int i;

	sio_iot = iot;
	sio_pc = pc;

	if (bus_space_map(sio_iot, IO_ICU1, 2, 0, &sio_ioh_icu1) ||
	    bus_space_map(sio_iot, IO_ICU2, 2, 0, &sio_ioh_icu2))
		panic("sio_intr_setup: can't map ICU I/O ports");

	for (i = 0; sio_elcr_setup_funcs[i] != NULL; i++)
		if ((*sio_elcr_setup_funcs[i])() == 0)
			break;
	if (sio_elcr_setup_funcs[i] == NULL)
		panic("sio_intr_setup: can't map ELCR");

#ifdef BROKEN_PROM_CONSOLE
	/*
	 * Remember the initial values, so we can restore them later.
	 */
	initial_ocw1[0] = bus_space_read_1(sio_iot, sio_ioh_icu1, 1);
	initial_ocw1[1] = bus_space_read_1(sio_iot, sio_ioh_icu2, 1);
	initial_elcr[0] = (*sio_read_elcr)(0);			/* XXX */
	initial_elcr[1] = (*sio_read_elcr)(1);			/* XXX */
	shutdownhook_establish(sio_intr_shutdown, 0);
#endif

	sio_intr = alpha_shared_intr_alloc(ICU_LEN, 8);

	/*
	 * set up initial values for interrupt enables.
	 */
	for (i = 0; i < ICU_LEN; i++) {
		alpha_shared_intr_set_maxstrays(sio_intr, i, STRAY_MAX);

		cp = alpha_shared_intr_string(sio_intr, i);
		sprintf(cp, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(sio_intr, i),
		    EVCNT_TYPE_INTR, NULL, "isa", cp);

		switch (i) {
		case 0:
		case 1:
		case 8:
		case 13:
			/*
			 * IRQs 0, 1, 8, and 13 must always be
			 * edge-triggered.
			 */
			sio_setirqstat(i, 0, IST_EDGE);
			alpha_shared_intr_set_dfltsharetype(sio_intr, i,
			    IST_EDGE);
			specific_eoi(i);
			break;

		case 2:
			/*
			 * IRQ 2 must be edge-triggered, and should be
			 * enabled (otherwise IRQs 8-15 are ignored).
			 */
			sio_setirqstat(i, 1, IST_EDGE);
			alpha_shared_intr_set_dfltsharetype(sio_intr, i,
			    IST_UNUSABLE);
			break;

		default:
			/*
			 * Otherwise, disable the IRQ and set its
			 * type to (effectively) "unknown."
			 */
			sio_setirqstat(i, 0, IST_NONE);
			alpha_shared_intr_set_dfltsharetype(sio_intr, i,
			    IST_NONE);
			specific_eoi(i);
			break;
		}
	}
}

#ifdef BROKEN_PROM_CONSOLE
void
sio_intr_shutdown(void *arg)
{
	/*
	 * Restore the initial values, to make the PROM happy.
	 */
	bus_space_write_1(sio_iot, sio_ioh_icu1, 1, initial_ocw1[0]);
	bus_space_write_1(sio_iot, sio_ioh_icu2, 1, initial_ocw1[1]);
	(*sio_write_elcr)(0, initial_elcr[0]);			/* XXX */
	(*sio_write_elcr)(1, initial_elcr[1]);			/* XXX */
}
#endif

const char *
sio_intr_string(void *v, int irq)
{
	static char irqstr[12];		/* 8 + 2 + NULL + sanity */

	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("sio_intr_string: bogus isa irq 0x%x", irq);

	sprintf(irqstr, "isa irq %d", irq);
	return (irqstr);
}

const struct evcnt *
sio_intr_evcnt(void *v, int irq)
{

	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("sio_intr_evcnt: bogus isa irq 0x%x", irq);

	return (alpha_shared_intr_evcnt(sio_intr, irq));
}

void *
sio_intr_establish(void *v, int irq, int type, int level, int (*fn)(void *), void *arg)
{
	void *cookie;

	if (irq > ICU_LEN || type == IST_NONE)
		panic("sio_intr_establish: bogus irq or type");

	cookie = alpha_shared_intr_establish(sio_intr, irq, type, level, fn,
	    arg, "isa irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(sio_intr, irq)) {
		scb_set(0x800 + SCB_IDXTOVEC(irq), sio_iointr, NULL,
		    level);
		sio_setirqstat(irq, 1,
		    alpha_shared_intr_get_sharetype(sio_intr, irq));
	}

	return (cookie);
}

void
sio_intr_disestablish(void *v, void *cookie)
{
	struct alpha_shared_intrhand *ih = cookie;
	int s, ist, irq = ih->ih_num;

	s = splhigh();

	/* Remove it from the link. */
	alpha_shared_intr_disestablish(sio_intr, cookie, "isa irq");

	/*
	 * Decide if we should disable the interrupt.  We must ensure
	 * that:
	 *
	 *	- An initially-enabled interrupt is never disabled.
	 *	- An initially-LT interrupt is never untyped.
	 */
	if (alpha_shared_intr_isactive(sio_intr, irq) == 0) {
		/*
		 * IRQs 0, 1, 8, and 13 must always be edge-triggered
		 * (see setup).
		 */
		switch (irq) {
		case 0:
		case 1:
		case 8:
		case 13:
			/*
			 * If the interrupt was initially level-triggered
			 * a warning was printed in setup.
			 */
			ist = IST_EDGE;
			break;

		default:
			ist = IST_NONE;
			break;
		}
		sio_setirqstat(irq, 0, ist);
		alpha_shared_intr_set_dfltsharetype(sio_intr, irq, ist);

		/* Release our SCB vector. */
		scb_free(0x800 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

void
sio_iointr(void *arg, unsigned long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x800);

#ifdef DIAGNOSTIC
	if (irq > ICU_LEN || irq < 0)
		panic("sio_iointr: irq out of range (%d)", irq);
#endif

	if (!alpha_shared_intr_dispatch(sio_intr, irq))
		alpha_shared_intr_stray(sio_intr, irq, "isa irq");
	else
		alpha_shared_intr_reset_strays(sio_intr, irq);

	/*
	 * Some versions of the machines which use the SIO
	 * (or is it some PALcode revisions on those machines?)
	 * require the non-specific EOI to be fed to the PIC(s)
	 * by the interrupt handler.
	 */
	specific_eoi(irq);
}

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN && (x) != 2)

int
sio_intr_alloc(void *v, int mask, int type, int *irq)
{
	int i, tmp, bestirq, count;
	struct alpha_shared_intrhand **p, *q;

	if (type == IST_NONE)
		panic("intr_alloc: bogus type");

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0xdef8;

	/*
	 * XXX some interrupts will be used later (6 for fdc, 12 for pms).
	 * the right answer is to do "breadth-first" searching of devices.
	 */
	mask &= 0xefbf;

	for (i = 0; i < ICU_LEN; i++) {
		if (LEGAL_IRQ(i) == 0 || (mask & (1<<i)) == 0)
			continue;

		switch(sio_intr[i].intr_sharetype) {
		case IST_NONE:
			/*
			 * if nothing's using the irq, just return it
			 */
			*irq = i;
			return (0);

		case IST_EDGE:
		case IST_LEVEL:
			if (type != sio_intr[i].intr_sharetype)
				continue;
			/*
			 * if the irq is shareable, count the number of other
			 * handlers, and if it's smaller than the last irq like
			 * this, remember it
			 *
			 * XXX We should probably also consider the
			 * interrupt level and stick IPL_TTY with other
			 * IPL_TTY, etc.
			 */
			for (p = &TAILQ_FIRST(&sio_intr[i].intr_q), tmp = 0;
			     (q = *p) != NULL; p = &TAILQ_NEXT(q, ih_q), tmp++)
				;
			if ((bestirq == -1) || (count > tmp)) {
				bestirq = i;
				count = tmp;
			}
			break;

		case IST_PULSE:
			/* this just isn't shareable */
			continue;
		}
	}

	if (bestirq == -1)
		return (1);

	*irq = bestirq;

	return (0);
}

static void
specific_eoi(int irq)
{
	if (irq > 7)
		bus_space_write_1(sio_iot,
		    sio_ioh_icu2, 0, 0x60 | (irq & 0x07));	/* XXX */
	bus_space_write_1(sio_iot, sio_ioh_icu1, 0, 0x60 | (irq > 7 ? 2 : irq));
}
