/* $NetBSD: mainbus.c,v 1.1.2.2 2011/02/08 18:05:06 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1.2.2 2011/02/08 18:05:06 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <machine/autoconf.h>

/* Definition of the mainbus driver. */
static int	mbmatch(struct device *, struct cfdata *, void *);
static void	mbattach(struct device *, struct device *, void *);
static int	mbprint(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mbmatch, mbattach, NULL, NULL);

static int mainbus_found;

static int
mbmatch(struct device *parent, struct cfdata *cf, void *aux)
{

	if (mainbus_found)
		return (0);

	return (1);
}

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args ma;

	mainbus_found = 1;

	printf("\n");

	/* Interrupt initialization, phase 1 */
	intr_init(1);

 	ma.ma_name = "cpu";
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);

	ma.ma_name = platform.iobus;
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);
}

static int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		return (QUIET);
	return (UNCONF);
}
