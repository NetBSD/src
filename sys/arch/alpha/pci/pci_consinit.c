/* $NetBSD: pci_consinit.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996, 1997 Carnegie-Mellon University.
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

#include "opt_kgdb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_consinit.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/bus.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "pckbd.h"

#ifndef CONSPEED
#define	CONSPEED	TTYDEF_SPEED
#endif
static int comcnrate = CONSPEED;

#ifdef KGDB
#include <machine/db_machdep.h>

static const char *kgdb_devlist[] = {
	"com",
	NULL,
};
#endif /* KGDB */

void
pci_consinit(pci_chipset_tag_t disp_pc,
    bus_space_tag_t disp_iot, bus_space_tag_t disp_memt,
    bus_space_tag_t isa_iot, bus_space_tag_t isa_memt)
{
	const struct ctb *ctb;

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);

	switch (ctb->ctb_term_type) {
	case CTB_PRINTERPORT:
		/* serial console ... */
		/* XXX */
		/*
		 * Delay to allow PROM putchars to complete.
		 * FIFO depth * character time,
		 * character time = (1000000 / (defaultrate / 10))
		 */
		DELAY(160000000 / comcnrate);

		if (comcnattach(isa_iot, 0x3f8, comcnrate, COM_FREQ,
				COM_TYPE_NORMAL,
				(TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)) {
			panic("can't init serial console");
		}
		break;

	case CTB_GRAPHICS:
#if NPCKBD > 0
		/* display console ... */
		/* XXX */
		(void) pckbc_cnattach(isa_iot, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT, 0);

		switch (CTB_TURBOSLOT_TYPE(ctb->ctb_turboslot)) {
		case CTB_TURBOSLOT_TYPE_ISA:
		case CTB_TURBOSLOT_TYPE_EISA:
			isa_display_console(isa_iot, isa_memt);
			break;

		case CTB_TURBOSLOT_TYPE_PCI:
		default:
			pci_display_console(disp_iot, disp_memt, disp_pc,
			    CTB_TURBOSLOT_BUS(ctb->ctb_turboslot),
			    CTB_TURBOSLOT_SLOT(ctb->ctb_turboslot), 0);
			break;
		}
#else
		panic("not configured to use display && keyboard console");
#endif
		break;

	default:
		printf("ctb->ctb_term_type = 0x%lx\n", ctb->ctb_term_type);
		printf("ctb->ctb_turboslot = 0x%lx\n", ctb->ctb_turboslot);

		panic("%s: unknown console type %ld", __func__,
		    ctb->ctb_term_type);
	}
#ifdef KGDB
	/* Attach the KGDB device. */
	alpha_kgdb_init(kgdb_devlist, isa_iot);
#endif /* KGDB */
}
