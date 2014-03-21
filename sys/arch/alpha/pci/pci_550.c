/* $NetBSD: pci_550.c,v 1.37 2014/03/21 16:39:29 christos Exp $ */

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Gallatin.
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

__KERNEL_RCSID(0, "$NetBSD: pci_550.c,v 1.37 2014/03/21 16:39:29 christos Exp $");

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

#include <alpha/pci/pci_550.h>

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

int	dec_550_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *dec_550_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *dec_550_intr_evcnt(void *, pci_intr_handle_t);
void	*dec_550_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	dec_550_intr_disestablish(void *, void *);

void	*dec_550_pciide_compat_intr_establish(void *, device_t,
	    const struct pci_attach_args *, int, int (*)(void *), void *);

#define	DEC_550_PCI_IRQ_BEGIN	8
#define	DEC_550_MAX_IRQ		(64 - DEC_550_PCI_IRQ_BEGIN)

/*
 * The Miata has a Pyxis, which seems to have problems with stray
 * interrupts.  Work around this by just ignoring strays.
 */
#define	PCI_STRAY_MAX		0

/*
 * Some Miata models, notably models with a Cypress PCI-ISA bridge, have
 * a PCI device (the OHCI USB controller) with interrupts tied to ISA IRQ
 * lines.  This IRQ is encoded as: line = FLAG | isa_irq. Usually FLAG
 * is 0xe0, however, it can be 0xf0.  We don't allow 0xf0 | irq15.
 */
#define	DEC_550_LINE_IS_ISA(line)	((line) >= 0xe0 && (line) <= 0xfe)
#define	DEC_550_LINE_ISA_IRQ(line)	((line) & 0x0f)

struct alpha_shared_intr *dec_550_pci_intr;

void	dec_550_iointr(void *arg, unsigned long vec);
void	dec_550_intr_enable(int irq);
void	dec_550_intr_disable(int irq);

void
pci_550_pickintr(struct cia_config *ccp)
{
	bus_space_tag_t iot = &ccp->cc_iot;
	pci_chipset_tag_t pc = &ccp->cc_pc;
	char *cp;
	int i;

	pc->pc_intr_v = ccp;
	pc->pc_intr_map = dec_550_intr_map;
	pc->pc_intr_string = dec_550_intr_string;
	pc->pc_intr_evcnt = dec_550_intr_evcnt;
	pc->pc_intr_establish = dec_550_intr_establish;
	pc->pc_intr_disestablish = dec_550_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    dec_550_pciide_compat_intr_establish;

	/*
	 * DEC 550's interrupts are enabled via the Pyxis interrupt
	 * mask register.  Nothing to map.
	 */

	for (i = 0; i < DEC_550_MAX_IRQ; i++)
		dec_550_intr_disable(i);

#define PCI_550_IRQ_STR	8
	dec_550_pci_intr = alpha_shared_intr_alloc(DEC_550_MAX_IRQ,
	    PCI_550_IRQ_STR);
	for (i = 0; i < DEC_550_MAX_IRQ; i++) {
		alpha_shared_intr_set_maxstrays(dec_550_pci_intr, i,
		    PCI_STRAY_MAX);
		alpha_shared_intr_set_private(dec_550_pci_intr, i, ccp);
		
		cp = alpha_shared_intr_string(dec_550_pci_intr, i);
		snprintf(cp, PCI_550_IRQ_STR, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    dec_550_pci_intr, i), EVCNT_TYPE_INTR, NULL,
		    "dec_550", cp);
	}

#if NSIO
	sio_intr_setup(pc, iot);
#endif
}

int
dec_550_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
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
		printf("dec_550_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * There are two main variants of Miata: Miata 1 (Intel SIO)
	 * and Miata {1.5,2} (Cypress).
	 *
	 * The Miata 1 has a CMD PCI IDE wired to compatibility mode at
	 * device 4 of bus 0.  This variant apparently also has the
	 * Pyxis DMA bug.
	 *
	 * On the Miata 1.5 and Miata 2, the Cypress PCI-ISA bridge lives
	 * on device 7 of bus 0.  This device has PCI IDE wired to
	 * compatibility mode on functions 1 and 2.
	 *
	 * There will be no interrupt mapping for these devices, so just
	 * bail out now.
	 */
	if (bus == 0) {
		if ((hwrpb->rpb_variation & SV_ST_MASK) < SV_ST_MIATA_1_5) {
			/* Miata 1 */
			if (device == 7)
				panic("dec_550_intr_map: SIO device");
			else if (device == 4)
				return (1);
		} else {
			/* Miata 1.5 or Miata 2 */
			if (device == 7) {
				if (function == 0)
					panic("dec_550_intr_map: SIO device");
				if (function == 1 || function == 2)
					return (1);
			}
		}
	}

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("dec_550_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

#if NSIO == 0
	if (DEC_550_LINE_IS_ISA(line)) {
		printf("dec_550_intr_map: ISA IRQ %d for %d/%d/%d\n",
		    DEC_550_LINE_ISA_IRQ(line), bus, device, function);
		return (1);
	}
#endif

	if (DEC_550_LINE_IS_ISA(line) == 0 && line >= DEC_550_MAX_IRQ) {
		printf("dec_550_intr_map: irq %d out of range %d/%d/%d\n",
		    line, bus, device, function);
		return (1);
	}
	*ihp = line;
	return (0);
}

const char *
dec_550_intr_string(void *ccv, pci_intr_handle_t ih, char *buf, size_t len)
{
#if 0
	struct cia_config *ccp = ccv;
#endif

#if NSIO
	if (DEC_550_LINE_IS_ISA(ih))
		return sio_intr_string(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(ih), buf, len);
#endif

	if (ih >= DEC_550_MAX_IRQ)
		panic("%s: bogus 550 IRQ 0x%lx", __func__, ih);
	snprintf(buf, len, "dec 550 irq %ld", ih);
	return buf;
}

const struct evcnt *
dec_550_intr_evcnt(void *ccv, pci_intr_handle_t ih)
{
#if 0
	struct cia_config *ccp = ccv;
#endif

#if NSIO
	if (DEC_550_LINE_IS_ISA(ih))
		return (sio_intr_evcnt(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(ih)));
#endif

	if (ih >= DEC_550_MAX_IRQ)
		panic("%s: bogus 550 IRQ 0x%lx", __func__, ih);

	return (alpha_shared_intr_evcnt(dec_550_pci_intr, ih));
}

void *
dec_550_intr_establish(void *ccv, pci_intr_handle_t ih, int level, int (*func)(void *), void *arg)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	void *cookie;

#if NSIO
	if (DEC_550_LINE_IS_ISA(ih))
		return (sio_intr_establish(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(ih), IST_LEVEL, level, func, arg));
#endif

	if (ih >= DEC_550_MAX_IRQ)
		panic("dec_550_intr_establish: bogus dec 550 IRQ 0x%lx", ih);

	cookie = alpha_shared_intr_establish(dec_550_pci_intr, ih, IST_LEVEL,
	    level, func, arg, "dec 550 irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(dec_550_pci_intr, ih)) {
		scb_set(0x900 + SCB_IDXTOVEC(ih), dec_550_iointr, NULL,
		    level);
		dec_550_intr_enable(ih);
	}
	return (cookie);
}

void
dec_550_intr_disestablish(void *ccv, void *cookie)
{
	struct cia_config *ccp = ccv;
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

#if NSIO
	/*
	 * We have to determine if this is an ISA IRQ or not!  We do this
	 * by checking to see if the intrhand points back to an intrhead
	 * that points to our cia_config.  If not, it's an ISA IRQ.  Pretty
	 * disgusting, eh?
	 */
	if (ih->ih_intrhead->intr_private != ccp) {
		sio_intr_disestablish(NULL /*XXX*/, cookie);
		return;
	}
#endif

	s = splhigh();

	alpha_shared_intr_disestablish(dec_550_pci_intr, cookie,
	    "dec 550 irq");
	if (alpha_shared_intr_isactive(dec_550_pci_intr, irq) == 0) {
		dec_550_intr_disable(irq);
		alpha_shared_intr_set_dfltsharetype(dec_550_pci_intr, irq,
		    IST_NONE);
		scb_free(0x900 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

void *
dec_550_pciide_compat_intr_establish(void *v, device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	void *cookie = NULL;
	int bus, irq;
	char buf[64];

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
	    PCIIDE_CHANNEL_NAME(chan), sio_intr_string(NULL /*XXX*/, irq,
	    buf, sizeof(buf)));
#endif
	return (cookie);
}

void
dec_550_iointr(void *arg, unsigned long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x900);

	if (irq >= DEC_550_MAX_IRQ)
		panic("550_iointr: vec 0x%lx out of range", vec);

	if (!alpha_shared_intr_dispatch(dec_550_pci_intr, irq)) {
		alpha_shared_intr_stray(dec_550_pci_intr, irq,
		    "dec 550 irq");
		if (ALPHA_SHARED_INTR_DISABLE(dec_550_pci_intr, irq))
			dec_550_intr_disable(irq);
	} else
		alpha_shared_intr_reset_strays(dec_550_pci_intr, irq);
}

void
dec_550_intr_enable(int irq)
{

	cia_pyxis_intr_enable(irq + DEC_550_PCI_IRQ_BEGIN, 1);
}

void
dec_550_intr_disable(int irq)
{

	cia_pyxis_intr_enable(irq + DEC_550_PCI_IRQ_BEGIN, 0);
}
