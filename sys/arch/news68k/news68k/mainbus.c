/*	$NetBSD: mainbus.c,v 1.2.6.2 2000/11/20 20:16:16 bouyer Exp $	*/

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

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <news68k/news68k/machid.h>

struct mainbus_softc {
	struct device sc_dev;
};

/* Definition of the mainbus driver. */
static int	mainbus_match __P((struct device *, struct cfdata *, void *));
static void	mainbus_attach __P((struct device *, struct device *, void *));
static int	mainbus_search __P((struct device *, struct cfdata *, void *));
static int	mainbus_print __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mainbus_match, mainbus_attach
};

static int mainbus_found;

static int
mainbus_match(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{

	if (mainbus_found)
		return 0;

	return 1;
}

static void
mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args ma;

	mainbus_found = 1;
	printf("\n");

	config_search(mainbus_search, self, &ma);
}

static int
mainbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	ma->ma_name = cf->cf_driver->cd_name;
	ma->ma_systype = cf->cf_systype;

	if ((*cf->cf_attach->ca_match)(parent, cf, ma) > 0)
		config_attach(parent, cf, ma, mainbus_print);

	return 0;
}

static int
mainbus_print(aux, cp)
	void *aux;
	const char *cp;
{
	struct mainbus_attach_args *ma = aux;

	if (cp)
		printf("%s at %s", ma->ma_name, cp);

	return (UNCONF);
}
