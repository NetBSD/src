/*	$NetBSD: mainbus.c,v 1.1 1995/02/13 23:07:04 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>

struct mainbus_softc {
	struct	device sc_dv;
	struct	abus sc_bus;
};

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, void *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, char *));
struct cfdriver mainbuscd =
    { NULL, "mainbus", mbmatch, mbattach, DV_DULL,
	sizeof (struct mainbus_softc) };

void	mb_intr_establish __P((struct confargs *, int (*)(void *), void *));
void	mb_intr_disestablish __P((struct confargs *));
caddr_t	mb_cvtaddr __P((struct confargs *));
int	mb_matchname __P((struct confargs *, char *));

/*
 * The devices that might be hanging off of the mainbus.
 * XXX Needs rethinking for multi-cpu support, if that happens.
 */
struct confargs mainbusdevs[] = {
        { "cpu", 0, },
        { "tc", 0, },
};
int nmainbusdevs = sizeof (mainbusdevs) / sizeof (mainbusdevs[0]);

static int
mbmatch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;

	/*
	 * Only one mainbus, but some people are stupid...
	 */	
	if (cf->cf_unit > 0)
		return(0);

	/*
	 * That one mainbus is always here.
	 */
	return(1);
}

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	int i;

	printf("\n");

	sc->sc_bus.ab_dv = (struct device *)sc;
	sc->sc_bus.ab_type = BUS_MAIN;
	sc->sc_bus.ab_intr_establish = mb_intr_establish;
	sc->sc_bus.ab_intr_disestablish = mb_intr_disestablish;
	sc->sc_bus.ab_cvtaddr = mb_cvtaddr;
	sc->sc_bus.ab_matchname = mb_matchname;

	for (i = 0; i < nmainbusdevs; i++) {
		mainbusdevs[i].ca_bus = &sc->sc_bus;
		config_found(self, &mainbusdevs[i], mbprint);
	}
}

static int
mbprint(aux, pnp)
	void *aux;
	char *pnp;
{

	if (pnp)
		return (QUIET);
	return (UNCONF);
}

void
mb_intr_establish(ca, handler, val)
	struct confargs *ca;
	int (*handler) __P((void *));
	void *val;
{

	panic("can never mb_intr_establish");
}

void
mb_intr_disestablish(ca)
	struct confargs *ca;
{

	panic("can never mb_intr_disestablish");
}

caddr_t
mb_cvtaddr(ca)
	struct confargs *ca;
{

	return (NULL);
}

int
mb_matchname(ca, name)
	struct confargs *ca;
	char *name;
{

	return (strcmp(name, ca->ca_name) == 0);
}
