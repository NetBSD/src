/*	$NetBSD: mainbus.c,v 1.8 2003/07/15 02:59:31 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.8 2003/07/15 02:59:31 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>
#include <machine/apbus.h>

struct mainbus_softc {
	struct device sc_dev;
};

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

CFATTACH_DECL(mainbus, sizeof(struct mainbus_softc),
    mbmatch, mbattach, NULL, NULL);

/* There can be only one. */
static int mainbus_found;

static int
mbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (mainbus_found)
		return 0;

	return 1;
}

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	register struct device *mb = self;
	struct confargs nca;

	mainbus_found = 1;
	printf("\n");

	nca.ca_name = "cpu";
	config_found(mb, &nca, mbprint);

	/* XXX */
	if (_sip != NULL) {
		nca.ca_name = "ap";
		config_found(self, &nca, NULL);
	} else {
		nca.ca_name = "hb";
		config_found(self, &nca, NULL);
	}
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		return QUIET;
	return UNCONF;
}
