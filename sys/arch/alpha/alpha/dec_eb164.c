/* $NetBSD: dec_eb164.c,v 1.65 2025/03/09 01:06:41 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_eb164.c,v 1.65 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

void dec_eb164_init(void);
static void dec_eb164_cons_init(void);
static void dec_eb164_device_register(device_t, void *);

void
dec_eb164_init(void)
{

	platform.family = "EB164";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX Don't know the system variations, yet. */
		platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "cia";
	platform.cons_init = dec_eb164_cons_init;
	platform.device_register = dec_eb164_device_register;

	/*
	 * EB164 systems have a 2MB secondary cache.
	 */
	uvmexp.ncolors = atop(2 * 1024 * 1024);
}

static void
dec_eb164_cons_init(void)
{
	struct cia_config *ccp;
	extern struct cia_config cia_configuration;

	ccp = &cia_configuration;
	cia_init(ccp);

	pci_consinit(&ccp->cc_pc, &ccp->cc_iot, &ccp->cc_memt,
	    &ccp->cc_iot, &ccp->cc_memt);
}

static void
dec_eb164_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}
