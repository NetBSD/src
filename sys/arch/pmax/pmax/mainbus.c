/* $NetBSD: mainbus.c,v 1.26.2.7 1999/11/19 11:06:29 nisimura Exp $ */

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

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mbmatch, mbattach
};

static int mainbus_found;

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

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args ma;

	mainbus_found = 1;

	printf("\n");

	/*
	 * if we ever support multi-processor DECsystem (5800 family),
	 * the Alpha port's mainbus.c has an example of attaching
	 * multiple CPUs.
	 *
	 * For now, we only have one. Attach it directly.
	 */
 	ma.ma_name = "cpu";
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);

	ma.ma_name = platform.iobus;
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);
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
