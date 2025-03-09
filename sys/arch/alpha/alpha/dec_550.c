/* $NetBSD: dec_550.c,v 1.39 2025/03/09 01:06:41 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_550.c,v 1.39 2025/03/09 01:06:41 thorpej Exp $");

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

/* Write this to Pyxis General Purpose Output to turn off the power. */
#define	DEC_550_PYXIS_GPO_POWERDOWN	0x00000400

void dec_550_init(void);
static void dec_550_cons_init(void);
static void dec_550_device_register(device_t, void *);
static void dec_550_powerdown(void);

void
dec_550_init(void)
{

	platform.family = "Digital Personal Workstation";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX Don't know the system variations, yet. */
		platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "cia";
	platform.cons_init = dec_550_cons_init;
	platform.device_register = dec_550_device_register;
	platform.powerdown = dec_550_powerdown;

	/*
	 * If Miata systems have a secondary cache, it's 2MB.
	 */
	uvmexp.ncolors = atop(2 * 1024 * 1024);
}

static void
dec_550_cons_init(void)
{
	struct cia_config *ccp;
	extern struct cia_config cia_configuration;

	ccp = &cia_configuration;
	cia_init(ccp);

	pci_consinit(&ccp->cc_pc, &ccp->cc_iot, &ccp->cc_memt,
	    &ccp->cc_iot, &ccp->cc_memt);
}

static void
dec_550_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}

static void
dec_550_powerdown(void)
{

	REGVAL(PYXIS_GPO) = DEC_550_PYXIS_GPO_POWERDOWN;
	alpha_mb();
}
