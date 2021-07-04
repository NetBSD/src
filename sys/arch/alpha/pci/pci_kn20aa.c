/* $NetBSD: pci_kn20aa.c,v 1.59 2021/07/04 22:42:36 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_kn20aa.c,v 1.59 2021/07/04 22:42:36 thorpej Exp $");

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

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include "sio.h"
#if NSIO > 0 || NPCEB > 0
#include <alpha/pci/siovar.h>
#endif

static int	dec_kn20aa_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

#define	KN20AA_PCEB_IRQ	31
#define	KN20AA_MAX_IRQ	32
#define	PCI_STRAY_MAX	5

static void	kn20aa_enable_intr(pci_chipset_tag_t pc, int irq);
static void	kn20aa_disable_intr(pci_chipset_tag_t pc, int irq);

static void
pci_kn20aa_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	int i;

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_kn20aa_intr_map;
	pc->pc_intr_string = alpha_pci_generic_intr_string;
	pc->pc_intr_evcnt = alpha_pci_generic_intr_evcnt;
	pc->pc_intr_establish = alpha_pci_generic_intr_establish;
	pc->pc_intr_disestablish = alpha_pci_generic_intr_disestablish;

	/* Not supported on KN20AA. */
	pc->pc_pciide_compat_intr_establish = NULL;

	pc->pc_intr_desc = "kn20aa";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = KN20AA_MAX_IRQ;

	pc->pc_intr_enable = kn20aa_enable_intr;
	pc->pc_intr_disable = kn20aa_disable_intr;

	for (i = 0; i < KN20AA_MAX_IRQ; i++) {
		kn20aa_disable_intr(pc, i);
	}

	alpha_pci_intr_alloc(pc, PCI_STRAY_MAX);

#if NSIO > 0 || NPCEB > 0
	sio_intr_setup(pc, iot);
	kn20aa_enable_intr(pc, KN20AA_PCEB_IRQ);
#endif
}
ALPHA_PCI_INTR_INIT(ST_DEC_KN20AA, pci_kn20aa_pickintr)

static int
dec_kn20aa_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device;
	int kn20aa_irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("dec_kn20aa_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	/*
	 * Slot->interrupt translation.  Appears to work, though it
	 * may not hold up forever.
	 *
	 * The DEC engineers who did this hardware obviously engaged
	 * in random drug testing.
	 */
	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	switch (device) {
	case 11:
	case 12:
		kn20aa_irq = ((device - 11) + 0) * 4;
		break;

	case 7:
		kn20aa_irq = 8;
		break;

	case 9:
		kn20aa_irq = 12;
		break;

	case 6:					/* 21040 on AlphaStation 500 */
		kn20aa_irq = 13;
		break;

	case 8:
		kn20aa_irq = 16;
		break;

	default:
	        printf("dec_kn20aa_intr_map: weird device number %d\n",
		    device);
	        return 1;
	}

	kn20aa_irq += buspin - 1;
	if (kn20aa_irq > KN20AA_MAX_IRQ)
		panic("dec_kn20aa_intr_map: kn20aa_irq too large (%d)",
		    kn20aa_irq);

	alpha_pci_intr_handle_init(ihp, kn20aa_irq, 0);
	return (0);
}

static void
kn20aa_enable_intr(pci_chipset_tag_t pc __unused, int irq)
{

	/*
	 * From disassembling small bits of the OSF/1 kernel:
	 * the following appears to enable a given interrupt request.
	 * "blech."  I'd give valuable body parts for better docs or
	 * for a good decompiler.
	 */
	alpha_mb();
	REGVAL(0x8780000000L + 0x40L) |= (1 << irq);	/* XXX */
	alpha_mb();
}

static void
kn20aa_disable_intr(pci_chipset_tag_t pc __unused, int irq)
{

	alpha_mb();
	REGVAL(0x8780000000L + 0x40L) &= ~(1 << irq);	/* XXX */
	alpha_mb();
}
