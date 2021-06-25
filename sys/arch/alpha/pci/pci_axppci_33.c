/* $NetBSD: pci_axppci_33.c,v 1.41 2021/06/25 13:41:33 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Jeffrey Hsu and Chris G. Demetriou
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

__KERNEL_RCSID(0, "$NetBSD: pci_axppci_33.c,v 1.41 2021/06/25 13:41:33 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/lcavar.h>

#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

static int	dec_axppci_33_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void
pci_axppci_33_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_axppci_33_intr_map;
	pc->pc_intr_string = sio_pci_intr_string;
	pc->pc_intr_evcnt = sio_pci_intr_evcnt;
	pc->pc_intr_establish = sio_pci_intr_establish;
	pc->pc_intr_disestablish = sio_pci_intr_disestablish;

	/* Not supported on AXPpci33. */
	pc->pc_pciide_compat_intr_establish = NULL;

#if NSIO
	sio_intr_setup(pc, iot);
#else
	panic("pci_axppci_33_pickintr: no I/O interrupt handler (no sio)");
#endif
}
ALPHA_PCI_INTR_INIT(ST_DEC_AXPPCI_33, pci_axppci_33_pickintr)

int
dec_axppci_33_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, pirq;

#ifndef DIAGNOSTIC
	pirq = 0;				/* XXX gcc -Wuninitialized */
#endif

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("dec_axppci_33_intr_map: bad interrupt pin %d\n",
		    buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	switch (device) {
	case 6:					/* NCR SCSI */
		pirq = 3;
		break;

	case 11:				/* slot 1 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 0;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 2;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 1;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_axppci_33_intr_map: bogus PCI pin %d",
			    buspin);
#endif
		};
		break;

	case 12:				/* slot 2 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 1;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 0;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 2;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_axppci_33_intr_map: bogus PCI pin %d",
			    buspin);
#endif
		};
		break;

	case 8:				/* slot 3 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 2;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 1;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 0;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_axppci_33_intr_map bogus: PCI pin %d",
			    buspin);
#endif
		};
		break;

	default:
	        printf("dec_axppci_33_intr_map: weird device number %d\n",
		    device);
	        return 1;
	}

#if 0
	printf("dec_axppci_33_intr_map: device %d pin %c: pirq %d\n",
		device, '@' + buspin, pirq);
#endif

	return sio_pirq_intr_map(pc, pirq, ihp);
}
