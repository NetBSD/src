/* $NetBSD: pci_eb164.c,v 1.50 2021/07/04 22:42:36 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_eb164.c,v 1.50 2021/07/04 22:42:36 thorpej Exp $");

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

static int	dec_eb164_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

#define	EB164_SIO_IRQ	4
#define	EB164_MAX_IRQ	24
#define	PCI_STRAY_MAX	5

static bus_space_tag_t eb164_intrgate_iot;
static bus_space_handle_t eb164_intrgate_ioh;

/* See pci_eb164_intr.s */
extern void	eb164_intr_enable(pci_chipset_tag_t, int irq);
extern void	eb164_intr_disable(pci_chipset_tag_t, int irq);

static void
pci_eb164_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	struct cia_config *ccp = core;
	int i;

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_eb164_intr_map;
	pc->pc_intr_string = alpha_pci_generic_intr_string;
	pc->pc_intr_evcnt = alpha_pci_generic_intr_evcnt;
	pc->pc_intr_establish = alpha_pci_generic_intr_establish;
	pc->pc_intr_disestablish = alpha_pci_generic_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    sio_pciide_compat_intr_establish;

	eb164_intrgate_iot = iot;
	if (bus_space_map(eb164_intrgate_iot, 0x804, 3, 0,
	    &eb164_intrgate_ioh) != 0)
		panic("pci_eb164_pickintr: couldn't map interrupt PLD");

	pc->pc_intr_desc = "eb164";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = EB164_MAX_IRQ;

	pc->pc_intr_enable = eb164_intr_enable;
	pc->pc_intr_disable = eb164_intr_disable;

	for (i = 0; i < EB164_MAX_IRQ; i++) {
		eb164_intr_disable(pc, i);
	}

	/*
	 * Systems with a Pyxis seem to have problems with
	 * stray interrupts, so just ignore them.
	 */
	alpha_pci_intr_alloc(pc,
	    (ccp->cc_flags & CCF_ISPYXIS) ? 0 : PCI_STRAY_MAX);

#if NSIO
	sio_intr_setup(pc, iot);
	eb164_intr_enable(pc, EB164_SIO_IRQ);
#endif
}
ALPHA_PCI_INTR_INIT(ST_EB164, pci_eb164_pickintr)

static int
dec_eb164_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, device, function;
	uint64_t variation;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
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

	return alpha_pci_generic_intr_map(pa, ihp);
}

#if 0		/* THIS DOES NOT WORK!  see pci_eb164_intr.S. */
uint8_t eb164_intr_mask[3] = { 0xff, 0xff, 0xff };

void
eb164_intr_enable(pci_chipset_tag_t pc __unused, int irq)
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
eb164_intr_disable(pci_chipset_tag_t pc __unused, int irq)
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
