/*	$NetBSD: mainbus.c,v 1.1 1998/02/18 13:48:32 tsubai Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>

struct mainbus_softc {
	struct device sc_dev;
};

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mbmatch, mbattach
};

extern struct cfdriver mainbus_cd;

static int
mbmatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;

	if (cf->cf_unit > 0)
		return 0;

	return 1;
}

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	register struct device *mb = self;
	struct confargs nca;

	extern int ncpus;

	printf("\n");

	/*
	 * if we ever support multi-CPU DEcstations (5800 family),
	 * the Alpha port's mainbus.c has an example of attaching
	 * multiple CPUs.
	 *
	 * For now, we only have one. Attach it directly.
	 */
	nca.ca_name = "cpu";
	nca.ca_slot = 0;
	nca.ca_offset = 0;
	nca.ca_addr = 0;
	config_found(mb, &nca, mbprint);

	nca.ca_name = "hb";
	config_found(self, &nca, NULL);
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
