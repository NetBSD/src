/* $NetBSD: pci_550.c,v 1.42 2021/07/04 22:42:36 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_550.c,v 1.42 2021/07/04 22:42:36 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
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

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

static int	dec_550_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *dec_550_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *dec_550_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static void	*dec_550_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *);
static void	dec_550_intr_disestablish(pci_chipset_tag_t, void *);

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

static void	dec_550_intr_enable(pci_chipset_tag_t, int irq);
static void	dec_550_intr_disable(pci_chipset_tag_t, int irq);

static void
pci_550_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	int i;

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_550_intr_map;
	pc->pc_intr_string = dec_550_intr_string;
	pc->pc_intr_evcnt = dec_550_intr_evcnt;
	pc->pc_intr_establish = dec_550_intr_establish;
	pc->pc_intr_disestablish = dec_550_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    sio_pciide_compat_intr_establish;

	pc->pc_intr_desc = "dec 550";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = DEC_550_MAX_IRQ;

	pc->pc_intr_enable = dec_550_intr_enable;
	pc->pc_intr_disable = dec_550_intr_disable;

	for (i = 0; i < DEC_550_MAX_IRQ; i++) {
		dec_550_intr_disable(pc, i);
	}

	alpha_pci_intr_alloc(pc, PCI_STRAY_MAX);

#if NSIO
	sio_intr_setup(pc, iot);
#endif
}
ALPHA_PCI_INTR_INIT(ST_DEC_550, pci_550_pickintr)

static int
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
	if (buspin < 0 || buspin > 4) {
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
	alpha_pci_intr_handle_init(ihp, line, 0);
	return (0);
}

static const char *
dec_550_intr_string(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    char * const buf, size_t const len)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_550_LINE_IS_ISA(irq))
		return sio_intr_string(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(irq), buf, len);
#endif

	return alpha_pci_generic_intr_string(pc, ih, buf, len);
}

static const struct evcnt *
dec_550_intr_evcnt(pci_chipset_tag_t const pc, pci_intr_handle_t const ih)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_550_LINE_IS_ISA(irq))
		return (sio_intr_evcnt(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(irq)));
#endif

	return alpha_pci_generic_intr_evcnt(pc, ih);
}

static void *
dec_550_intr_establish(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    int const level, int (*func)(void *), void *arg)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);

	if (DEC_550_LINE_IS_ISA(irq))
		return (sio_intr_establish(NULL /*XXX*/,
		    DEC_550_LINE_ISA_IRQ(irq), IST_LEVEL, level, flags,
		    func, arg));
#endif

	return alpha_pci_generic_intr_establish(pc, ih, level, func, arg);
}

static void
dec_550_intr_disestablish(pci_chipset_tag_t const pc, void * const cookie)
{
#if NSIO
	struct alpha_shared_intrhand * const ih = cookie;

	/*
	 * We have to determine if this is an ISA IRQ or not!  We do this
	 * by checking to see if the intrhand points back to an intrhead
	 * that points to our cia_config.  If not, it's an ISA IRQ.  Pretty
	 * disgusting, eh?
	 */
	if (ih->ih_intrhead->intr_private != pc->pc_intr_v) {
		sio_intr_disestablish(NULL /*XXX*/, cookie);
		return;
	}
#endif

	alpha_pci_generic_intr_disestablish(pc, cookie);
}

static void
dec_550_intr_enable(pci_chipset_tag_t const pc __unused, int const irq)
{
	cia_pyxis_intr_enable(irq + DEC_550_PCI_IRQ_BEGIN, 1);
}

static void
dec_550_intr_disable(pci_chipset_tag_t const pc __unused, int const irq)
{
	cia_pyxis_intr_enable(irq + DEC_550_PCI_IRQ_BEGIN, 0);
}
