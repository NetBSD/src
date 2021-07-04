/* $NetBSD: pci_1000.c,v 1.31 2021/07/04 22:42:36 thorpej Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is based on pci_kn20aa.c, written by Chris G. Demetriou at
 * Carnegie-Mellon University. Platform support for Mikasa and Mikasa/Pinnacle
 * (Pinkasa) by Ross Harvey with copyright assignment by permission of Avalon
 * Computer Systems, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: pci_1000.c,v 1.31 2021/07/04 22:42:36 thorpej Exp $");

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

#include "sio.h"
#if NSIO > 0 || NPCEB > 0
#include <alpha/pci/siovar.h>
#endif

static bus_space_tag_t another_mystery_icu_iot;
static bus_space_handle_t another_mystery_icu_ioh;

static int	dec_1000_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

#define	PCI_NIRQ	16
#define	PCI_STRAY_MAX	5

static void	dec_1000_enable_intr(pci_chipset_tag_t, int irq);
static void	dec_1000_disable_intr(pci_chipset_tag_t, int irq);
static void	pci_1000_imi(void);

static void
pci_1000_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{

	another_mystery_icu_iot = iot;

	if (bus_space_map(iot, 0x536, 2, 0, &another_mystery_icu_ioh))
		panic("pci_1000_pickintr");

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_1000_intr_map;
	pc->pc_intr_string = alpha_pci_generic_intr_string;
	pc->pc_intr_evcnt = alpha_pci_generic_intr_evcnt;
	pc->pc_intr_establish = alpha_pci_generic_intr_establish;
	pc->pc_intr_disestablish = alpha_pci_generic_intr_disestablish;

	pc->pc_pciide_compat_intr_establish = NULL;

	pc->pc_intr_desc = "dec 1000";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = PCI_NIRQ;

	pc->pc_intr_enable = dec_1000_enable_intr;
	pc->pc_intr_disable = dec_1000_disable_intr;

	pci_1000_imi();

	alpha_pci_intr_alloc(pc, PCI_STRAY_MAX);

#if NSIO > 0 || NPCEB > 0
	sio_intr_setup(pc, iot);
#endif
}
ALPHA_PCI_INTR_INIT(ST_DEC_1000, pci_1000_pickintr)

static int
dec_1000_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int	device = 0;	/* XXX gcc */

	if (buspin == 0)	/* No IRQ used. */
		return 1;
	if (!(1 <= buspin && buspin <= 4))
		goto bad;

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	switch(device) {
	case 6:
		if(buspin != 1)
			break;
		/* integrated ncr scsi */
		alpha_pci_intr_handle_init(ihp, 0xc, 0);
		return 0;
	case 11:
	case 12:
	case 13:
		alpha_pci_intr_handle_init(ihp,
		    (device - 11) * 4 + buspin - 1, 0);
		return 0;
	}

bad:	printf("dec_1000_intr_map: can't map dev %d pin %d\n", device, buspin);
	return 1;
}

/*
 * Read and write the mystery ICU IMR registers
 */

#define	IR() bus_space_read_2(another_mystery_icu_iot,		\
				another_mystery_icu_ioh, 0)

#define	IW(v) bus_space_write_2(another_mystery_icu_iot,	\
				another_mystery_icu_ioh, 0, (v))

/*
 * Enable and disable interrupts at the ICU level
 */

static void
dec_1000_enable_intr(pci_chipset_tag_t pc __unused, int irq)
{
	IW(IR() | 1 << irq);
}

static void
dec_1000_disable_intr(pci_chipset_tag_t pc __unused, int irq)
{
	IW(IR() & ~(1 << irq));
}

/*
 * Initialize mystery ICU
 */
static void
pci_1000_imi(void)
{
	IW(0);					/* XXX ?? */
}
