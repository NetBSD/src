/*	$NetBSD: mainbus.c,v 1.1.2.1 1999/12/27 18:33:02 wrstuden Exp $	*/

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

#include <news68k/news68k/machid.h>

struct mainbus_softc {
	struct device sc_dev;
};

/* Definition of the mainbus driver. */
static int	mainbus_match __P((struct device *, struct cfdata *, void *));
static void	mainbus_attach __P((struct device *, struct device *, void *));
static int	mainbus_print __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mainbus_match, mainbus_attach
};

#ifdef news1700
static	char *mainbusdevs_1700[] = {
	"hb",
#if 0 /* which model has this? */
	"vme",
#endif
	NULL
};
#endif

extern int news_model_id;

static int
mainbus_match(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;

	if (cf->cf_unit > 0)
		return 0;

	return 1;
}

static void
mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	char **devices;
	int i;

	printf("\n");

	/*
	 * Attach children appropriate for this CPU.
	 */
	switch (news_model_id) {
#ifdef news1700
	case NWS1710:
	case NWS1720:
	case NWS1750:
	case NWS1760:
	case NWS1510:
	case NWS1520:
	case NWS1530:
	case PWS1550:
	case PWS1560:
	case PWS1570:
	case NWS1580:
	case PWS1590:
	case NWS1410:
	case NWS1450:
	case NWS1460:
	case PWS1630:
		devices = mainbusdevs_1700;
		break;
#endif

	default:
		panic("mainbus_attach: impossible model type");
	}

	for (i = 0; devices[i] != NULL; ++i)
		(void)config_found(self, devices[i], mainbus_print);
}

static int
mainbus_print(aux, cp)
	void *aux;
	const char *cp;
{
	char *devname = aux;

	if (cp)
		printf("%s at %s", devname, cp);

	return (UNCONF);
}
