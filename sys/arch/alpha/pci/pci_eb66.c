/* $NetBSD: pci_eb66.c,v 1.23.6.1 2014/08/20 00:02:41 tls Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: pci_eb66.c,v 1.23.6.1 2014/08/20 00:02:41 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/lcareg.h>
#include <alpha/pci/lcavar.h>

#include <alpha/pci/pci_eb66.h>

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

int	dec_eb66_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *dec_eb66_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *dec_eb66_intr_evcnt(void *, pci_intr_handle_t);
void	*dec_eb66_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	dec_eb66_intr_disestablish(void *, void *);

#define	EB66_MAX_IRQ		32
#define	PCI_STRAY_MAX		5

struct alpha_shared_intr *eb66_pci_intr;

bus_space_tag_t eb66_intrgate_iot;
bus_space_handle_t eb66_intrgate_ioh;

void	eb66_iointr(void *arg, unsigned long vec);
extern void	eb66_intr_enable(int irq);  /* pci_eb66_intr.S */
extern void	eb66_intr_disable(int irq); /* pci_eb66_intr.S */

void
pci_eb66_pickintr(struct lca_config *lcp)
{
	bus_space_tag_t iot = &lcp->lc_iot;
	pci_chipset_tag_t pc = &lcp->lc_pc;
	char *cp;
	int i;

	pc->pc_intr_v = lcp;
	pc->pc_intr_map = dec_eb66_intr_map;
	pc->pc_intr_string = dec_eb66_intr_string;
	pc->pc_intr_evcnt = dec_eb66_intr_evcnt;
	pc->pc_intr_establish = dec_eb66_intr_establish;
	pc->pc_intr_disestablish = dec_eb66_intr_disestablish;

	/* Not supported on the EB66. */
	pc->pc_pciide_compat_intr_establish = NULL;

	eb66_intrgate_iot = iot;
	if (bus_space_map(eb66_intrgate_iot, 0x804, 3, 0,
	    &eb66_intrgate_ioh) != 0)
		panic("pci_eb66_pickintr: couldn't map interrupt PLD");
	for (i = 0; i < EB66_MAX_IRQ; i++)
		eb66_intr_disable(i);	

#define PCI_EB66_IRQ_STR	8
	eb66_pci_intr = alpha_shared_intr_alloc(EB66_MAX_IRQ, PCI_EB66_IRQ_STR);
	for (i = 0; i < EB66_MAX_IRQ; i++) {
		alpha_shared_intr_set_maxstrays(eb66_pci_intr, i,
			PCI_STRAY_MAX);
		
		cp = alpha_shared_intr_string(eb66_pci_intr, i);
		snprintf(cp, PCI_EB66_IRQ_STR, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    eb66_pci_intr, i), EVCNT_TYPE_INTR, NULL,
		    "eb66", cp);
	}

#if NSIO
	sio_intr_setup(pc, iot);
#endif
}

int
dec_eb66_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin, line = pa->pa_intrline;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_eb66_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("dec_eb66_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

	if (line >= EB66_MAX_IRQ)
		panic("dec_eb66_intr_map: eb66 irq too large (%d)",
		    line);

	*ihp = line;
	return (0);
}

const char *
dec_eb66_intr_string(void *lcv, pci_intr_handle_t ih, char *buf, size_t len)
{
	if (ih >= EB66_MAX_IRQ)
		panic("%s: bogus eb66 IRQ 0x%lx", __func__, ih);
	snprintf(buf, len, "eb66 irq %ld", ih);
	return buf;
}

const struct evcnt *
dec_eb66_intr_evcnt(void *lcv, pci_intr_handle_t ih)
{

	if (ih >= EB66_MAX_IRQ)
		panic("dec_eb66_intr_evcnt: bogus eb66 IRQ 0x%lx", ih);
	return (alpha_shared_intr_evcnt(eb66_pci_intr, ih));
}

void *
dec_eb66_intr_establish(void *lcv, pci_intr_handle_t ih, int level, int (*func)(void *), void *arg)
{
	void *cookie;

	if (ih >= EB66_MAX_IRQ)
		panic("dec_eb66_intr_establish: bogus eb66 IRQ 0x%lx", ih);

	cookie = alpha_shared_intr_establish(eb66_pci_intr, ih, IST_LEVEL,
	    level, func, arg, "eb66 irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(eb66_pci_intr, ih)) {
		scb_set(0x900 + SCB_IDXTOVEC(ih), eb66_iointr, NULL,
		    level);
		eb66_intr_enable(ih);
	}
	return (cookie);
}

void
dec_eb66_intr_disestablish(void *lcv, void *cookie)
{
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

	s = splhigh();

	alpha_shared_intr_disestablish(eb66_pci_intr, cookie,
	    "eb66 irq");
	if (alpha_shared_intr_isactive(eb66_pci_intr, irq) == 0) {
		eb66_intr_disable(irq);
		alpha_shared_intr_set_dfltsharetype(eb66_pci_intr, irq,
		    IST_NONE);
		scb_free(0x900 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

void
eb66_iointr(void *arg, unsigned long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x900);

	if (!alpha_shared_intr_dispatch(eb66_pci_intr, irq)) {
		alpha_shared_intr_stray(eb66_pci_intr, irq,
		    "eb66 irq");
		if (ALPHA_SHARED_INTR_DISABLE(eb66_pci_intr, irq))
			eb66_intr_disable(irq);
	} else
		alpha_shared_intr_reset_strays(eb66_pci_intr, irq);
}

#if 0		/* THIS DOES NOT WORK!  see pci_eb66_intr.S. */
uint8_t eb66_intr_mask[3] = { 0xff, 0xff, 0xff };

void
eb66_intr_enable(int irq)
{
	int byte = (irq / 8), bit = (irq % 8);

#if 1
	printf("eb66_intr_enable: enabling %d (%d:%d)\n", irq, byte, bit);
#endif
	eb66_intr_mask[byte] &= ~(1 << bit);

	bus_space_write_1(eb66_intrgate_iot, eb66_intrgate_ioh, byte,
	    eb66_intr_mask[byte]);
}

void
eb66_intr_disable(int irq)
{
	int byte = (irq / 8), bit = (irq % 8);

#if 1
	printf("eb66_intr_disable: disabling %d (%d:%d)\n", irq, byte, bit);
#endif
	eb66_intr_mask[byte] |= (1 << bit);

	bus_space_write_1(eb66_intrgate_iot, eb66_intrgate_ioh, byte,
	    eb66_intr_mask[byte]);
}
#endif
