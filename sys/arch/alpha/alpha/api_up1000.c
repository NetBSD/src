/* $NetBSD: api_up1000.c,v 1.34 2025/03/09 01:06:41 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: api_up1000.c,v 1.34 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/alpha.h>
#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/irongatereg.h>
#include <alpha/pci/irongatevar.h>

void api_up1000_init(void);
static void api_up1000_cons_init(void);
static void api_up1000_device_register(device_t, void *);

void
api_up1000_init(void)
{

	platform.family = "Alpha Processor, Inc. UP1000";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX Don't know the system variations, yet. */
		platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "irongate";
	platform.cons_init = api_up1000_cons_init;
	platform.device_register = api_up1000_device_register;
	platform.page_physload = irongate_page_physload;
}

static void
api_up1000_cons_init(void)
{
	struct irongate_config *icp;
	extern struct irongate_config irongate_configuration;

	icp = &irongate_configuration;
	irongate_init(icp);

	pci_consinit(&icp->ic_pc, &icp->ic_iot, &icp->ic_memt,
	    &icp->ic_iot, &icp->ic_memt);
}

static void
api_up1000_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}
