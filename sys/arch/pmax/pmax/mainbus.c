/*	$NetBSD: mainbus.c,v 1.31 1999/05/26 04:23:59 nisimura Exp $	*/

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

#include <machine/sysconf.h>
#include <machine/autoconf.h>
#include <pmax/pmax/pmaxtype.h>

#include <dev/tc/tcvar.h>	/* XXX */
#include "tc.h"			/* XXX Is TURBOchannel configured? */

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mbmatch, mbattach
};

static int
mbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/*
	 * Only one mainbus, but some people are stupid...
	 */
	if (cf->cf_unit > 0)
		return (0);

	/*
	 * That one mainbus is always here.
	 */
	return (1);
}

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs nca;

	printf("\n");

	/*
	 * if we ever support multi-processor DECsystem (5800 family),
	 * the Alpha port's mainbus.c has an example of attaching
	 * multiple CPUs.
	 *
	 * For now, we only have one. Attach it directly.
	 */
 	nca.ca_name = "cpu";
	nca.ca_slot = 0;
	config_found(self, &nca, mbprint);

#if NTC > 0
	if (systype == DS_3MAXPLUS || systype == DS_3MAX ||
	    systype == DS_3MIN || systype == DS_MAXINE) {
		/*
		 * This system might have a turbochannel.
		 * Call the TC subr code to look for one
		 * and if found, to configure it.
		 */
		config_tcbus(self, systype /* XXX */, mbprint);
	}
#endif /* NTC */

	if (systype == DS_PMAX || systype == DS_MIPSMATE) {
		nca.ca_name = platform.iobus;
		nca.ca_slot = 0;
		config_found(self, &nca, mbprint);
	}
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		return (QUIET);
	return (UNCONF);
}
