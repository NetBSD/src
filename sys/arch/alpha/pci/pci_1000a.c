/* $NetBSD: pci_1000a.c,v 1.33 2021/07/04 22:42:36 thorpej Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is based on pci_kn20aa.c, written by Chris G. Demetriou at
 * Carnegie-Mellon University. Platform support for Noritake, Pintake, and
 * Corelle by Ross Harvey with copyright assignment by permission of Avalon
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

__KERNEL_RCSID(0, "$NetBSD: pci_1000a.c,v 1.33 2021/07/04 22:42:36 thorpej Exp $");

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

#define	PCI_NIRQ	32
#define	PCI_STRAY_MAX	5

#define IMR2IRQ(bn) ((bn) - 1)
#define IRQ2IMR(irq) ((irq) + 1)

static bus_space_tag_t mystery_icu_iot;
static bus_space_handle_t mystery_icu_ioh[2];

static int	dec_1000a_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void	dec_1000a_enable_intr(pci_chipset_tag_t, int irq);
static void	dec_1000a_disable_intr(pci_chipset_tag_t, int irq);
static void	pci_1000a_imi(void);

static void
pci_1000a_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{

	mystery_icu_iot = iot;

	if (bus_space_map(iot, 0x54a, 2, 0, mystery_icu_ioh + 0)
	||  bus_space_map(iot, 0x54c, 2, 0, mystery_icu_ioh + 1))
		panic("pci_1000a_pickintr");

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_1000a_intr_map;
	pc->pc_intr_string = alpha_pci_generic_intr_string;
	pc->pc_intr_evcnt = alpha_pci_generic_intr_evcnt;
	pc->pc_intr_establish = alpha_pci_generic_intr_establish;
	pc->pc_intr_disestablish = alpha_pci_generic_intr_disestablish;

	pc->pc_pciide_compat_intr_establish = NULL;

	pc->pc_intr_desc = "dec 1000a";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = PCI_NIRQ;

	pc->pc_intr_enable = dec_1000a_enable_intr;
	pc->pc_intr_disable = dec_1000a_disable_intr;

	pci_1000a_imi();

	alpha_pci_intr_alloc(pc, PCI_STRAY_MAX);

#if NSIO > 0 || NPCEB > 0
	sio_intr_setup(pc, iot);
#endif
}
ALPHA_PCI_INTR_INIT(ST_DEC_1000A, pci_1000a_pickintr)

int
dec_1000a_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int imrbit, device = 0;	/* XXX gcc */
	/*
	 * Get bit number in mystery ICU imr
	 */
	static const signed char imrmap[][4] = {
#		define	IRQSPLIT(o) { (o), (o)+1, (o)+16, (o)+16+1 }
#		define	IRQNONE		 { 0, 0, 0, 0 }
		/*  0  */ { 1, 0, 0, 0 },	/* Noritake and Pintake */
		/*  1  */ IRQSPLIT(8),
		/*  2  */ IRQSPLIT(10),
		/*  3  */ IRQSPLIT(12),
		/*  4  */ IRQSPLIT(14),
		/*  5  */ { 1, 0, 0, 0 },	/* Corelle */
		/*  6  */ { 10, 0, 0, 0 },	/* Corelle */
		/*  7  */ IRQNONE,
		/*  8  */ { 1, 0, 0, 0 },	/* isp behind ppb */
		/*  9  */ IRQNONE,
		/* 10  */ IRQNONE,
		/* 11  */ IRQSPLIT(2),
		/* 12  */ IRQSPLIT(4),
		/* 13  */ IRQSPLIT(6),
		/* 14  */ IRQSPLIT(8)		/* Corelle */
	};

	if (buspin == 0)	/* No IRQ used. */
		return 1;
	if (!(1 <= buspin && buspin <= 4))
		goto bad;
	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (0 <= device && device < sizeof imrmap / sizeof imrmap[0]) {
		if (device == 0)
			printf("dec_1000a_intr_map: ?! UNEXPECTED DEV 0\n");
		imrbit = imrmap[device][buspin - 1];
		if (imrbit) {
			alpha_pci_intr_handle_init(ihp, IMR2IRQ(imrbit), 0);
			return 0;
		}
	}
bad:	printf("dec_1000a_intr_map: can't map dev %d pin %d\n", device, buspin);
	return 1;
}

/*
 * Read and write the mystery ICU IMR registers
 */

#define	IR(h) bus_space_read_2(mystery_icu_iot, mystery_icu_ioh[h], 0)
#define	IW(h, v) bus_space_write_2(mystery_icu_iot, mystery_icu_ioh[h], 0, (v))

/*
 * Enable and disable interrupts at the ICU level
 */

static void
dec_1000a_enable_intr(pci_chipset_tag_t pc __unused, int irq)
{
	int imrval = IRQ2IMR(irq);
	int i = imrval >= 16;

	IW(i, IR(i) | 1 << (imrval & 0xf));
}

static void
dec_1000a_disable_intr(pci_chipset_tag_t pc __unused, int irq)
{
	int imrval = IRQ2IMR(irq);
	int i = imrval >= 16;

	IW(i, IR(i) & ~(1 << (imrval & 0xf)));
}

/*
 * Initialize mystery ICU
 */
static void
pci_1000a_imi(void)
{
	IW(0, IR(0) & 1);
	IW(1, IR(0) & 3);
}
