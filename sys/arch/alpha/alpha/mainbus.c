/* $NetBSD: mainbus.c,v 1.25 1998/05/13 23:38:26 thorpej Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.25 1998/05/13 23:38:26 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/conf.h>

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mbmatch, mbattach
};

/* There can be only one. */
int	mainbus_found;

static int
mbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	if (mainbus_found)
		return (0);

	return (1);
}

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs nca;
	struct pcs *pcsp;
	int i, cpuattachcnt;
	extern int ncpus;

	mainbus_found = 1;

	printf("\n");

	/*
	 * Try to find and attach all of the CPUs in the machine.
	 */
	cpuattachcnt = 0;
	for (i = 0; i < hwrpb->rpb_pcs_cnt; i++) {

		pcsp = (struct pcs *)((char *)hwrpb + hwrpb->rpb_pcs_off +
		    (i * hwrpb->rpb_pcs_size));
		if ((pcsp->pcs_flags & PCS_PP) == 0)
			continue;

		nca.ca_name = "cpu";
		nca.ca_slot = i;
		nca.ca_offset = 0;
		if (config_found(self, &nca, mbprint) != NULL)
			cpuattachcnt++;
	}
	if (ncpus != cpuattachcnt)
		printf("WARNING: %d cpus in machine, %d attached\n",
			ncpus, cpuattachcnt);

	if (platform.iobus != NULL) {
		nca.ca_name = (char *) platform.iobus;
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		config_found(self, &nca, mbprint);
	}
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct confargs *ca = aux;

	if (pnp)
		printf("%s at %s", ca->ca_name, pnp);

	return (UNCONF);
}
