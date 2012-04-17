/* $NetBSD: pci_eb164.c,v 1.43.2.1 2012/04/17 00:05:57 yamt Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_eb164.c,v 1.43.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include <alpha/pci/pci_eb164.h>

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

int	dec_eb164_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
const char *dec_eb164_intr_string(void *, pci_intr_handle_t);
const struct evcnt *dec_eb164_intr_evcnt(void *, pci_intr_handle_t);
void	*dec_eb164_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	dec_eb164_intr_disestablish(void *, void *);

void	*dec_eb164_pciide_compat_intr_establish(void *, device_t,
	    const struct pci_attach_args *, int, int (*)(void *), void *);

#define	EB164_SIO_IRQ	4
#define	EB164_MAX_IRQ	24
#define	PCI_STRAY_MAX	5

struct alpha_shared_intr *eb164_pci_intr;

bus_space_tag_t eb164_intrgate_iot;
bus_space_handle_t eb164_intrgate_ioh;

void	eb164_iointr(void *arg, unsigned long vec);
extern void	eb164_intr_enable(int irq);	/* pci_eb164_intr.S */
extern void	eb164_intr_disable(int irq);	/* pci_eb164_intr.S */

void
pci_eb164_pickintr(struct cia_config *ccp)
{
	bus_space_tag_t iot = &ccp->cc_iot;
	pci_chipset_tag_t pc = &ccp->cc_pc;
	char *cp;
	int i;

	pc->pc_intr_v = ccp;
	pc->pc_intr_map = dec_eb164_intr_map;
	pc->pc_intr_string = dec_eb164_intr_string;
	pc->pc_intr_evcnt = dec_eb164_intr_evcnt;
	pc->pc_intr_establish = dec_eb164_intr_establish;
	pc->pc_intr_disestablish = dec_eb164_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    dec_eb164_pciide_compat_intr_establish;

	eb164_intrgate_iot = iot;
	if (bus_space_map(eb164_intrgate_iot, 0x804, 3, 0,
	    &eb164_intrgate_ioh) != 0)
		panic("pci_eb164_pickintr: couldn't map interrupt PLD");
	for (i = 0; i < EB164_MAX_IRQ; i++)
		eb164_intr_disable(i);	

	eb164_pci_intr = alpha_shared_intr_alloc(EB164_MAX_IRQ, 8);
	for (i = 0; i < EB164_MAX_IRQ; i++) {
		/*
		 * Systems with a Pyxis seem to have problems with
		 * stray interrupts, so just ignore them.  Sigh,
		 * I hate buggy hardware.
		 */
		alpha_shared_intr_set_maxstrays(eb164_pci_intr, i,
			(ccp->cc_flags & CCF_ISPYXIS) ? 0 : PCI_STRAY_MAX);

		cp = alpha_shared_intr_string(eb164_pci_intr, i);
		sprintf(cp, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    eb164_pci_intr, i), EVCNT_TYPE_INTR, NULL,
		    "eb164", cp);
	}

#if NSIO
	sio_intr_setup(pc, iot);
	eb164_intr_enable(EB164_SIO_IRQ);
#endif
}

int
dec_eb164_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin, line = pa->pa_intrline;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, device, function;
	uint64_t variation;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_eb164_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	variation = hwrpb->rpb_variation & SV_ST_MASK;

	/*
	 *
	 * The AlphaPC 164 and AlphaPC 164LX have a CMD PCI IDE controller
	 * at bus 0 device 11.  These are wired to compatibility mode,
	 * so do not map their interrupts.
	 *
	 * The AlphaPC 164SX has PCI IDE on functions 1 and 2 of the
	 * Cypress PCI-ISA bridge at bus 0 device 8.  These, too, are
	 * wired to compatibility mode.
	 *
	 * Real EB164s have ISA IDE on the Super I/O chip.
	 */
	if (bus == 0) {
		if (variation >= SV_ST_ALPHAPC164_366 &&
		    variation <= SV_ST_ALPHAPC164LX_600) {
			if (device == 8)
				panic("dec_eb164_intr_map: SIO device");
			if (device == 11)
				return (1);
		} else if (variation >= SV_ST_ALPHAPC164SX_400 &&
			   variation <= SV_ST_ALPHAPC164SX_600) {
			if (device == 8) {
				if (function == 0)
					panic("dec_eb164_intr_map: SIO device");
				return (1);
			}
		} else {
			if (device == 8)
				panic("dec_eb164_intr_map: SIO device");
		}
	}

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("dec_eb164_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

	if (line > EB164_MAX_IRQ)
		panic("dec_eb164_intr_map: eb164 irq too large (%d)",
		    line);

	*ihp = line;
	return (0);
}

const char *
dec_eb164_intr_string(void *ccv, pci_intr_handle_t ih)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	static char irqstr[15];          /* 11 + 2 + NULL + sanity */

	if (ih > EB164_MAX_IRQ)
	        panic("dec_eb164_intr_string: bogus eb164 IRQ 0x%lx", ih);
	sprintf(irqstr, "eb164 irq %ld", ih);
	return (irqstr);
}

const struct evcnt *
dec_eb164_intr_evcnt(void *ccv, pci_intr_handle_t ih)
{
#if 0
	struct cia_config *ccp = ccv;
#endif

	if (ih > EB164_MAX_IRQ)
		panic("dec_eb164_intr_string: bogus eb164 IRQ 0x%lx", ih);
	return (alpha_shared_intr_evcnt(eb164_pci_intr, ih));
}

void *
dec_eb164_intr_establish(void *ccv, pci_intr_handle_t ih, int level, int (*func)(void *), void *arg)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	void *cookie;

	if (ih > EB164_MAX_IRQ)
		panic("dec_eb164_intr_establish: bogus eb164 IRQ 0x%lx", ih);

	cookie = alpha_shared_intr_establish(eb164_pci_intr, ih, IST_LEVEL,
	    level, func, arg, "eb164 irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(eb164_pci_intr, ih)) {
		scb_set(0x900 + SCB_IDXTOVEC(ih), eb164_iointr, NULL,
		   level);
		eb164_intr_enable(ih);
	}
	return (cookie);
}

void
dec_eb164_intr_disestablish(void *ccv, void *cookie)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

	s = splhigh();

	alpha_shared_intr_disestablish(eb164_pci_intr, cookie,
	    "eb164 irq");
	if (alpha_shared_intr_isactive(eb164_pci_intr, irq) == 0) {
		eb164_intr_disable(irq);
		alpha_shared_intr_set_dfltsharetype(eb164_pci_intr, irq,
		    IST_NONE);
		scb_free(0x900 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

void *
dec_eb164_pciide_compat_intr_establish(void *v, device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	void *cookie = NULL;
	int bus, irq;

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	irq = PCIIDE_COMPAT_IRQ(chan);
#if NSIO
	cookie = sio_intr_establish(NULL /*XXX*/, irq, IST_EDGE, IPL_BIO,
	    func, arg);
	if (cookie == NULL)
		return (NULL);
	aprint_normal_dev(dev, "%s channel interrupting at %s\n",
	    PCIIDE_CHANNEL_NAME(chan), sio_intr_string(NULL /*XXX*/, irq));
#endif
	return (cookie);
}

void
eb164_iointr(void *arg, unsigned long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x900);

	if (!alpha_shared_intr_dispatch(eb164_pci_intr, irq)) {
		alpha_shared_intr_stray(eb164_pci_intr, irq,
		    "eb164 irq");
		if (ALPHA_SHARED_INTR_DISABLE(eb164_pci_intr, irq))
			eb164_intr_disable(irq);
	} else
		alpha_shared_intr_reset_strays(eb164_pci_intr, irq);
}

#if 0		/* THIS DOES NOT WORK!  see pci_eb164_intr.S. */
uint8_t eb164_intr_mask[3] = { 0xff, 0xff, 0xff };

void
eb164_intr_enable(int irq)
{
	int byte = (irq / 8), bit = (irq % 8);

#if 1
	printf("eb164_intr_enable: enabling %d (%d:%d)\n", irq, byte, bit);
#endif
	eb164_intr_mask[byte] &= ~(1 << bit);

	bus_space_write_1(eb164_intrgate_iot, eb164_intrgate_ioh, byte,
	    eb164_intr_mask[byte]);
}

void
eb164_intr_disable(int irq)
{
	int byte = (irq / 8), bit = (irq % 8);

#if 1
	printf("eb164_intr_disable: disabling %d (%d:%d)\n", irq, byte, bit);
#endif
	eb164_intr_mask[byte] |= (1 << bit);

	bus_space_write_1(eb164_intrgate_iot, eb164_intrgate_ioh, byte,
	    eb164_intr_mask[byte]);
}
#endif
