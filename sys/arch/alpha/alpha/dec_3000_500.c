/* $NetBSD: dec_3000_500.c,v 1.49 2025/03/09 01:06:41 thorpej Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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
/*
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_3000_500.c,v 1.49 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/tcdsvar.h>
#include <alpha/tc/tc_3000_500.h>

#include <machine/z8530var.h>
#include <dev/tc/zs_ioasicvar.h>

#include "wsdisplay.h"

void dec_3000_500_init(void);
static void dec_3000_500_cons_init(void);
static void dec_3000_500_device_register(device_t, void *);

static const char dec_3000_500_sp[] = "DEC 3000/400 (\"Sandpiper\")";
static const char dec_3000_500_sf[] = "DEC 3000/500 (\"Flamingo\")";

const struct alpha_variation_table dec_3000_500_variations[] = {
	{ SV_ST_SANDPIPER, dec_3000_500_sp },
	{ SV_ST_FLAMINGO, dec_3000_500_sf },
	{ SV_ST_HOTPINK, "DEC 3000/500X (\"Hot Pink\")" },
	{ SV_ST_FLAMINGOPLUS, "DEC 3000/800 (\"Flamingo+\")" },
	{ SV_ST_SANDPLUS, "DEC 3000/600 (\"Sandpiper+\")" },
	{ SV_ST_SANDPIPER45, "DEC 3000/700 (\"Sandpiper45\")" },
	{ SV_ST_FLAMINGO45, "DEC 3000/900 (\"Flamingo45\")" },
	{ 0, NULL },
};

void
dec_3000_500_init(void)
{
	uint64_t variation;

	platform.family = "DEC 3000/500 (\"Flamingo\")";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if (variation == SV_ST_ULTRA) {
			/* These are really the same. */
			variation = SV_ST_FLAMINGOPLUS;
		}
		if ((platform.model = alpha_variation_name(variation,
		    dec_3000_500_variations)) == NULL) {
			/*
			 * This is how things used to be done.
			 */
			if (variation == SV_ST_RESERVED) {
				if (hwrpb->rpb_variation & SV_GRAPHICS)
					platform.model = dec_3000_500_sf;
				else
					platform.model = dec_3000_500_sp;
			} else
				platform.model = alpha_unknown_sysname();
		}
	}

	platform.iobus = "tcasic";
	platform.cons_init = dec_3000_500_cons_init;
	platform.device_register = dec_3000_500_device_register;
}

static void
dec_3000_500_cons_init(void)
{
	struct ctb *ctb;

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);

	switch (ctb->ctb_term_type) {
	case CTB_GRAPHICS:
#if NWSDISPLAY > 0
		/* display console ... */
		if (zs_ioasic_lk201_cnattach(0x1e0000000, 0x00180000, 0) == 0 &&
		    tc_3000_500_fb_cnattach(
		     CTB_TURBOSLOT_SLOT(ctb->ctb_turboslot)) == 0) {
			break;
		}
#endif
		printf("consinit: Unable to init console on keyboard and ");
		printf("TURBOchannel slot 0x%lx.\n", ctb->ctb_turboslot);
		printf("Using serial console.\n");
		/* FALLTHROUGH */

	case CTB_PRINTERPORT:
		/* serial console ... */
		/*
		 * XXX This could stand some cleanup...
		 */
		{
			/*
			 * Delay to allow PROM putchars to complete.
			 * FIFO depth * character time,
			 * character time = (1000000 / (defaultrate / 10))
			 */
			DELAY(160000000 / 9600);	/* XXX */

			/*
			 * Console is channel B of the second SCC.
			 * XXX Should use ctb_line_off to get the
			 * XXX line parameters--these are the defaults.
			 */
			zs_ioasic_cnattach(0x1e0000000, 0x00180000, 1);
			break;
		}

	default:
		printf("ctb->ctb_term_type = 0x%lx\n", ctb->ctb_term_type);
		printf("ctb->ctb_turboslot = 0x%lx\n", ctb->ctb_turboslot);
		panic("consinit: unknown console type %lu",
		    ctb->ctb_term_type);
		/* NOTREACHED */
	}
}

static void
dec_3000_500_device_register(device_t dev, void *aux)
{
	tc_find_bootdev(dev, aux);
}
