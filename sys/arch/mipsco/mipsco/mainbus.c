/*	$NetBSD: mainbus.c,v 1.7.92.1 2009/05/13 17:18:03 jym Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * DECstation port: Jonathan Stone
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7.92.1 2009/05/13 17:18:03 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>

struct mainbus_softc {
	struct device sc_dev;
};

/* Definition of the mainbus driver. */
static int	mbmatch(struct device *, struct cfdata *, void *);
static void	mbattach(struct device *, struct device *, void *);
static int	mbprint(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct mainbus_softc),
    mbmatch, mbattach, NULL, NULL);

static int mb_attached;

static int
mbmatch(struct device *parent, struct cfdata *cfdata, void *aux)
{

	if (mb_attached)
		return 0;

	return 1;
}

static void
mbattach(struct device *parent, struct device *self, void *aux)
{
	register struct device *mb = self;
	struct confargs nca;

	mb_attached = 1;

	printf("\n");

	nca.ca_name = "cpu";
	nca.ca_addr = 0;
	config_found(mb, &nca, mbprint);

	nca.ca_name = "obio";
	nca.ca_addr = 0;
	config_found(self, &nca, NULL);

	nca.ca_name = "isabus";		/* XXX */
	nca.ca_addr = 0;
	config_found(self, &nca, NULL);

}

static int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		return QUIET;
	return UNCONF;
}
