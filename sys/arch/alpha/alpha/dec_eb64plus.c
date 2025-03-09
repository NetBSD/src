/* $NetBSD: dec_eb64plus.c,v 1.44 2025/03/09 01:06:41 thorpej Exp $ */

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
/*
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_eb64plus.c,v 1.44 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>

void dec_eb64plus_init(void);
static void dec_eb64plus_cons_init(void);
static void dec_eb64plus_device_register(device_t, void *);

const struct alpha_variation_table dec_eb64plus_variations[] = {
	{ 0, "DEC EB64+" },
	{ 0, NULL },
};

void
dec_eb64plus_init(void)
{
	uint64_t variation;

	platform.family = "EB64+";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_eb64plus_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "apecs";
	platform.cons_init = dec_eb64plus_cons_init;
	platform.device_register = dec_eb64plus_device_register;

	/*
	 * EB64+ systems can have 512K, 1M, or 2M secondary
	 * caches.  Default to middle-of-the-road.
	 *
	 * XXX Need to dynamically size it!
	 */
	uvmexp.ncolors = atop(1 * 1024 * 1024);
}

static void
dec_eb64plus_cons_init(void)
{
	struct apecs_config *acp;
	extern struct apecs_config apecs_configuration;

	acp = &apecs_configuration;
	apecs_init(acp);

	pci_consinit(&acp->ac_pc, &acp->ac_iot, &acp->ac_memt,
	    &acp->ac_iot, &acp->ac_memt);
}

static void
dec_eb64plus_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}
